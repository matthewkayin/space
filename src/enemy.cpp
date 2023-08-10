#include "enemy.hpp"

#include "raycast.hpp"
#include "resource.hpp"
#include "shader.hpp"
#include "globals.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum EnemyAnimation {
    ENEMY_ANIMATION_IDLE,
    ENEMY_ANIMATION_ATTACK,
    ENEMY_ANIMATION_DIE
};

EnemyBulletHole::EnemyBulletHole(glm::vec3 position, glm::vec3 normal) {
    this->position = position;
    this->normal = normal;

    animation.add_animation(0, {
        .start_frame = 0,
        .end_frame = 2,
        .frame_time = 2.0f
    });
    animation.set_animation(0);
}

void EnemyBulletHole::update(float delta) {
    if (animation.is_finished) {
        return;
    }

    animation.update(delta);
}

void EnemyBulletHole::render() {
    if (animation.is_finished) {
        return;
    }

    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_wasp_bullet_hole]));
    glm::mat4 model = glm::inverse(glm::lookAt(position, position + normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), false);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation.frame);

    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_wasp_bullet_hole);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

Enemy::Enemy(unsigned int id) {
    position = glm::vec3(0.0f, 1.0f, -1.0f);
    direction = glm::vec3(0.0f, 0.0f, 1.0f);
    facing_direction = direction;
    angle = 0.0f;

    animation.add_animation(ENEMY_ANIMATION_IDLE, {
        .start_frame = 0,
        .end_frame = 2,
        .frame_time = 2.0f,
    });
    animation.add_animation(ENEMY_ANIMATION_ATTACK, {
        .start_frame = 16,
        .end_frame = 18,
        .frame_time = 2.0f
    });
    animation.add_animation(ENEMY_ANIMATION_DIE, {
        .start_frame = 19,
        .end_frame = 21,
        .frame_time = 2.0f
    });

    hurtbox_extents = glm::vec2((100.0f / 2.0f) / (SCREEN_WIDTH / 2), (120.0f / 2.0f) / (SCREEN_HEIGHT / 2));
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 model = glm::inverse(glm::lookAt(position, position + facing_direction, up));
    hurtbox_raycast_plane = raycast_add_plane({
        .type = PLANE_TYPE_ENEMY,
        .id = id,
        .a = glm::vec3(model * glm::vec4(-hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f)),
        .b = glm::vec3(model * glm::vec4(hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f)),
        .c = glm::vec3(model * glm::vec4(hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f)),
        .d = glm::vec3(model * glm::vec4(-hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f)),
        .normal = facing_direction,
        .enabled = true
    });

    health = 3;
    is_dead = false;

    has_seen_player = false;
    hit_player = false;
    has_hit = false;
}

void Enemy::update(glm::vec3 player_position, float delta) {
    if (is_dead) {
        return;
    }

    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    hit_player = false;

    // determine facing direction and angle
    facing_direction = glm::normalize(glm::vec3(player_position.x, position.y, player_position.z) - position);
    float base_angle = atan2(direction.z, direction.x) * (180 / 3.14f);
    angle = (atan2(facing_direction.z, facing_direction.x) * (180 / 3.14f)) - base_angle;
    if (angle > 180.0f) {
        angle = -180.0f + (angle - 180.0f);
    } else if (angle < -180.0f) {
        angle = 180.0f - (-angle - 180.0f);
    }

    if (!has_seen_player && abs(angle) < 90.0f) {
        RaycastResult result = raycast_cast(position, glm::normalize(player_position - position), glm::length(player_position - position), true);
        if (!result.hit) {
            has_seen_player = true;
        }
    }

    if (has_seen_player && abs(angle) < 30.0f && animation.animation == ENEMY_ANIMATION_IDLE && glm::length(position - player_position) <= 1.0f) {
        animation.set_animation(ENEMY_ANIMATION_ATTACK);
        has_hit = true;
    }

    if (has_seen_player) {
        direction = direction + ((facing_direction - direction) * 0.05f * delta);
        float dist2d = glm::length(glm::vec2(player_position.x, player_position.z) - glm::vec2(position.x, position.z));
        if (dist2d > 1.0f) {
            velocity += direction * std::min(dist2d, 0.1f * delta);
        }
        if (abs(position.y - player_position.y) > 0.2f) {
            float y_direction = 1.0f;
            if (player_position.y < position.y) {
                y_direction = -1.0f;
            }
            velocity.y += y_direction * std::min(0.05f * delta, abs(position.y - player_position.y));
        }

        RaycastResult result = raycast_cast(position, glm::normalize(velocity), 1.0f, true);
        float velocity_length = glm::length(velocity);
        unsigned int attempts = 1;
        while (result.hit && attempts < 5) {
            glm::vec3 plane_normal = raycast_planes[result.plane].normal;
            glm::vec3 velocity_in_wall_normal_direction = plane_normal * glm::dot(velocity, plane_normal);
            velocity -= velocity_in_wall_normal_direction;

            if (glm::length(velocity) == 0.0f) {
                break;
            }

            velocity = glm::normalize(velocity) * velocity_length;

            result = raycast_cast(position, glm::normalize(velocity), 1.0f, true);
            attempts++;
        }
        if (attempts == 5) {
            velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        position += velocity;
    }

    // update animation
    animation.update(delta);
    if (animation.animation == ENEMY_ANIMATION_DIE && animation.is_finished) {
        is_dead = true;
    }
    if (animation.animation == ENEMY_ANIMATION_ATTACK && animation.is_finished) {
        animation.set_animation(ENEMY_ANIMATION_IDLE);
    }
    if (has_hit && animation.animation == ENEMY_ANIMATION_ATTACK && animation.frame == 17 && glm::length(position - player_position) <= 1.0f) {
        hit_player = true;
        has_hit = false;
    }

    // update bullet holes
    std::vector<unsigned int> indices_to_remove;
    for (EnemyBulletHole& bullet_hole : bullet_holes) {
        bullet_hole.position += velocity;
        bullet_hole.update(delta);
    }
    for (unsigned int index: indices_to_remove) {
        bullet_holes.erase(bullet_holes.begin() + index);
    }
}

void Enemy::take_damage(RaycastResult& result, int amount) {
    health -= amount;
    if (health <= 0) {
        animation.set_animation(ENEMY_ANIMATION_DIE);
    } else {
        bullet_holes.push_back(EnemyBulletHole(result.point + (raycast_planes[hurtbox_raycast_plane].normal * 0.05f), raycast_planes[hurtbox_raycast_plane].normal));
    }
}

void Enemy::render() {
    if (is_dead) {
        return;
    }

    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_wasp]));

    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(up, facing_direction)) == 1.0f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    glm::mat4 model = glm::inverse(glm::lookAt(position, position + facing_direction, up));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(facing_direction));

    animation_offset = (unsigned int)(abs(angle) / 36.0f);
    flip_h = angle < 0.0f && animation_offset >= 1 && animation_offset <= 3;

    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), flip_h);

    unsigned int animation_frame = animation.frame;
    if (animation.animation == ENEMY_ANIMATION_IDLE) {
        animation_frame += animation_offset * 3;
    }

    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation_frame);

    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_wasp);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // update hurtbox plane
    raycast_planes[hurtbox_raycast_plane].a = glm::vec3(model * glm::vec4(-hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].b = glm::vec3(model * glm::vec4(hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].c = glm::vec3(model * glm::vec4(hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].d = glm::vec3(model * glm::vec4(-hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].normal = facing_direction;

    for (EnemyBulletHole& bullet_hole : bullet_holes) {
        bullet_hole.render();
    }
}
