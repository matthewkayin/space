#include "scene.hpp"

#include "enemy.hpp"
#include "player.hpp"
#include "globals.hpp"
#include "resource.hpp"
#include "level.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Player player;
std::vector<Enemy> enemies;
std::vector<EnemyBulletHole> enemy_bullet_holes;

void scene_init() {
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUseProgram(billboard_shader);
    glUniform1i(glGetUniformLocation(billboard_shader, "u_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "lighting_enabled"), true);
    glUniform2iv(glGetUniformLocation(billboard_shader, "screen_size"), 1, glm::value_ptr(screen_size));
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "screen_size"), 1, glm::value_ptr(screen_size));

    for (EnemySpawn& enemy_spawn : enemy_spawns) {
        Enemy enemy;
        enemy.position = enemy_spawn.position;
        enemy.direction = glm::vec3(enemy_spawn.direction.x, 0.0f, enemy_spawn.direction.y);
        enemies.push_back(enemy);
    }

    player.init();
}

void scene_update(float delta) {
    // update player
    player.update(delta);
    scene_move_and_slide(&player.position, &player.velocity, delta);
    if (player.raycast_result.hit) {
        const RaycastPlane& plane = raycast_planes[player.raycast_result.plane];
        if (plane.type == PLANE_TYPE_LEVEL) {
            sectors[plane.id].bullet_holes.push_back({
                .position = player.raycast_result.point + (plane.normal * 0.05f),
                .normal = plane.normal
            });
        } else if (plane.type == PLANE_TYPE_ENEMY) {
            enemy_bullet_holes.push_back(EnemyBulletHole(player.raycast_result.point + (plane.normal * 0.05f), plane.normal));
            enemies[plane.id].take_damage(1);
        }
    }

    // update enemies
    std::vector<unsigned int> indices_to_remove;
    for (unsigned int i = 0; i < enemies.size(); i++) {
        enemies[i].update(player.position, delta);

        if (enemies[i].is_dead) {
            indices_to_remove.push_back(i);
        }
        if (enemies[i].hit_player) {
            player.take_damage(7);
        }
    }
    for (unsigned int index : indices_to_remove) {
        raycast_planes.erase(raycast_planes.begin() + enemies[index].hurtbox_raycast_plane);
        enemies.erase(enemies.begin() + index);
    }
    indices_to_remove.clear();

    // update enemy bullet holes
    for (unsigned int i = 0; i < enemy_bullet_holes.size(); i++) {
        enemy_bullet_holes[i].update(delta);
        if (enemy_bullet_holes[i].animation.is_finished) {
            indices_to_remove.push_back(i);
        }
    }
    for (unsigned int index : indices_to_remove) {
        enemy_bullet_holes.erase(enemy_bullet_holes.begin() + index);
    }
}

void scene_move_and_slide(glm::vec3* position, glm::vec3* velocity, float delta) {
    // check collisions and move player
    if (glm::length(*velocity) == 0.0f) {
        return;
    }
    glm::vec3 actual_velocity = *velocity * delta;

    // check if within sector AABB
    std::vector<Sector*> nearby_sectors;
    for (unsigned int i = 0; i < sectors.size(); i++) {
        Sector* sector = &sectors[i];
        float padding = 1.0f;
        if (position->x < sector->aabb_top_left.x - padding || position->x > sector->aabb_bot_right.x + padding ||
            position->y < sector->floor_y || position->y > sector->ceiling_y ||
            position->z < sector->aabb_top_left.y - padding || position->z > sector->aabb_bot_right.y + padding) {
            continue;
        }
        nearby_sectors.push_back(sector);
    }

    // check floor / ceiling collisions
    glm::vec2 origin2d = glm::vec2(position->x, position->z);
    for (Sector* sector : nearby_sectors) {
        glm::vec2 raycast_start = sector->aabb_top_left - glm::vec2(1.0f, 1.0f);
        glm::vec2 raycast_direction = origin2d - raycast_start;
        unsigned int hits = 0;
        for (unsigned int wall = 0; wall < sector->vertices.size(); wall++) {
            float raycast_result = raycast_cast2d(raycast_start, raycast_direction, sector->vertices[wall], sector->vertices[(wall + 1) % sector->vertices.size()] - sector->vertices[wall]);
            if (raycast_result != -1.0f) {
                hits++;
            }
        }

        if (hits % 2 == 0) {
            continue;
        }

        float predicted_y = position->y + actual_velocity.y;
        if (predicted_y + 0.5f >= sector->ceiling_y) {
            glm::vec3 velocity_in_ceiling_normal_direction = glm::vec3(0.0f, -1.0f, 0.0f) * glm::dot(actual_velocity, glm::vec3(0.0f, -1.0f, 0.0f));
            *velocity -= velocity_in_ceiling_normal_direction;
            actual_velocity -= velocity_in_ceiling_normal_direction;
        } else if (predicted_y - 0.5f <= sector->floor_y) {
            glm::vec3 velocity_in_floor_normal_direction = glm::vec3(0.0f, 1.0f, 0.0f) * glm::dot(actual_velocity, glm::vec3(0.0f, 1.0f, 0.0f));
            *velocity -= velocity_in_floor_normal_direction;
            actual_velocity -= velocity_in_floor_normal_direction;
        }
    }

    // check wall collisions
    bool collided = true;
    unsigned int attempts = 0;
    while (collided && attempts < 10) {
        collided = false;
        attempts++;

        std::vector<glm::vec2> wall_a;
        std::vector<glm::vec2> wall_b;
        std::vector<glm::vec3> wall_normal;
        for (Sector* sector : nearby_sectors) {
            for (unsigned int wall = 0; wall < sector->vertices.size(); wall++) {
                if (!sector->walls[wall].exists) {
                    continue;
                }

                wall_a.push_back(sector->vertices[(wall + 1) % sector->vertices.size()]);
                wall_b.push_back(sector->vertices[wall]);
                wall_normal.push_back(sector->walls[wall].normal);
            }
        }
        for (Enemy& enemy : enemies) {
            RaycastPlane& enemy_plane = raycast_planes[enemy.hurtbox_raycast_plane];
            if (position->y >= enemy_plane.a.y && position->y <= enemy_plane.d.y) {
                wall_a.push_back(glm::vec2(enemy_plane.b.x, enemy_plane.b.z));
                wall_b.push_back(glm::vec2(enemy_plane.a.x, enemy_plane.a.z));
                wall_normal.push_back(enemy_plane.normal);
            }
        }

        for (unsigned int wall_index = 0; wall_index < wall_a.size(); wall_index++) {
            glm::vec2 velocity2d = glm::vec2(actual_velocity.x, actual_velocity.z);
            glm::vec2 predicted_origin2d = origin2d + velocity2d;

            glm::vec2 wallv = wall_a[wall_index] - wall_b[wall_index];
            glm::vec2 f = wall_b[wall_index] - predicted_origin2d;
            float a = glm::dot(wallv, wallv);
            float b = 2 * glm::dot(f, wallv);
            float c = glm::dot(f, f) - 0.25f;
            float discriminant = (b * b) - (4.0f * a * c);

            if (discriminant < 0.0f) {
                continue;
            }

            discriminant = sqrt(discriminant);
            float t1 = (-b - discriminant) / (2.0f * a);
            float t2 = (-b + discriminant) / (2.0f * a);

            if (!(t1 >= 0.0f && t1 <= 1.0f) && !(t2 >= 0.0f && t2 <= 1.0f)) {
                continue;
            }

            glm::vec3 velocity_in_wall_normal_direction = wall_normal[wall_index] * glm::dot(actual_velocity, wall_normal[wall_index]);
            *velocity -= velocity_in_wall_normal_direction;
            actual_velocity = actual_velocity - velocity_in_wall_normal_direction;
            collided = true;
        }
    }

    if (attempts == 5) {
        actual_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    *position += actual_velocity;
}

void scene_render() {
    glm::vec3 camera_direction = player.position + player.direction;
    glm::mat4 view;
    view = glm::lookAt(player.position, camera_direction, glm::vec3(player.basis[1]));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 100.0f);

    // prepare billboard shader
    glUseProgram(billboard_shader);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flashlight_on"), player.flashlight_on);
    glUniform3fv(glGetUniformLocation(billboard_shader, "view_pos"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.position"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.direction"), 1, glm::value_ptr(player.flashlight_direction));
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), false);

    level_render(view, projection, player.position, player.flashlight_direction, player.flashlight_on);

    // bind quad vertex

    for (Enemy& enemy : enemies) {
        enemy.render();
    }
    for (EnemyBulletHole& enemy_bullet_hole : enemy_bullet_holes) {
        enemy_bullet_hole.render();
    }

    player.render();
}
