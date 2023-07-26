#include "level.hpp"

#include "shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <cstdio>

const int TEXTURE_SIZE = 64;

float raycast(glm::vec2 a_origin, glm::vec2 a_direction, glm::vec2 b_origin, glm::vec2 b_direction) {
    glm::vec2 b_minus_a = b_origin - a_origin;
    float a_direction_cross_b_direction = (a_direction.x * b_direction.y) - (a_direction.y * b_direction.x);

    float a_time = ((b_minus_a.x * b_direction.y) - (b_minus_a.y * b_direction.x)) / a_direction_cross_b_direction;
    float b_time = ((b_minus_a.x * a_direction.y) - (b_minus_a.y * a_direction.x)) / a_direction_cross_b_direction;

    if (a_time < 0.0f || a_time > 1.0f || b_time < 0.0f || b_time > 1.0f) {
        return -1.0f;
    }

    return a_time;
}

Sector::Sector() {

}

void Sector::add_vertex(const glm::vec2 vertex, bool add_wall) {
    vertices.push_back(vertex);
    walls.push_back({
        .exists = add_wall,
        .normal = glm::vec3(0.0f, 0.0f, 0.0f)
    });
}

void Sector::init_buffers() {
    // walls
    for (unsigned int i = 0; i < vertices.size(); i++) {
        if (!walls[i].exists) {
            continue;
        }

        unsigned int end_index = (i + 1) % vertices.size();
        glm::vec3 wall_top_left = glm::vec3(vertices[i].x, ceiling_y, vertices[i].y);
        glm::vec3 wall_bot_left = glm::vec3(vertices[i].x, floor_y, vertices[i].y);
        glm::vec3 wall_top_right = glm::vec3(vertices[end_index].x, ceiling_y, vertices[end_index].y);
        glm::vec3 wall_bot_right = glm::vec3(vertices[end_index].x, floor_y, vertices[end_index].y);

        glm::vec3 wall_vertices[6] = {
            wall_top_left,
            wall_top_right,
            wall_bot_left,

            wall_top_right,
            wall_bot_right,
            wall_bot_left
        };

        glm::vec2 wall_scale = glm::vec2(glm::length(vertices[i] - vertices[end_index]), std::fabs(ceiling_y - floor_y));
        glm::vec2 texture_coordinates[6] = {
            glm::vec2(0.0f, wall_scale.y),
            glm::vec2(wall_scale.x, wall_scale.y),
            glm::vec2(0.0f, 0.0f),

            glm::vec2(wall_scale.x, wall_scale.y),
            glm::vec2(wall_scale.x, 0.0f),
            glm::vec2(0.0f, 0.0f)
        };

        for (unsigned int face = 0; face < 2; face++) {
            unsigned int base_index = face * 3;
            glm::vec3 face_normal = glm::normalize(glm::cross(wall_vertices[base_index + 2] - wall_vertices[base_index], wall_vertices[base_index + 1] - wall_vertices[base_index]));

            for (unsigned int j = 0; j < 3; j++) {
                vertex_data.push_back({
                    .position = wall_vertices[base_index + j],
                    .normal = face_normal,
                    .texture_index = 0,
                    .texture_coordinates = texture_coordinates[base_index + j]
                });
            }

            if (face == 0) {
                walls[i].normal = face_normal;
            }
        }
    }

    // determine AABB
    aabb_top_left = vertices[0];
    aabb_bot_right = vertices[0];
    for (unsigned int i = 1; i < vertices.size(); i++) {
        aabb_top_left.x = std::min(aabb_top_left.x, vertices[i].x);
        aabb_top_left.y = std::min(aabb_top_left.y, vertices[i].y);
        aabb_bot_right.x = std::max(aabb_bot_right.x, vertices[i].x);
        aabb_bot_right.y = std::max(aabb_bot_right.y, vertices[i].y);
    }
    aabb[0] = glm::vec4(aabb_top_left.x, ceiling_y, aabb_top_left.y, 1.0f);
    aabb[1] = glm::vec4(aabb_bot_right.x, ceiling_y, aabb_top_left.y, 1.0f);
    aabb[2] = glm::vec4(aabb_top_left.x, ceiling_y, aabb_bot_right.y, 1.0f);
    aabb[3] = glm::vec4(aabb_bot_right.x, ceiling_y, aabb_bot_right.y, 1.0f);
    aabb[4] = glm::vec4(aabb_top_left.x, floor_y, aabb_top_left.y, 1.0f);
    aabb[5] = glm::vec4(aabb_bot_right.x, floor_y, aabb_top_left.y, 1.0f);
    aabb[6] = glm::vec4(aabb_top_left.x, floor_y, aabb_bot_right.y, 1.0f);
    aabb[7] = glm::vec4(aabb_bot_right.x, floor_y, aabb_bot_right.y, 1.0f);

    // ceiling and floor
    glm::vec2 ceiling_scale = glm::vec2(std::fabs(aabb_bot_right.x - aabb_top_left.x), std::fabs(aabb_top_left.y - aabb_bot_right.y));
    for (unsigned int i = 2; i < vertices.size(); i++) {
        glm::vec3 triangle_vertices[3] = {
            glm::vec3(vertices[0].x, ceiling_y, vertices[0].y),
            glm::vec3(vertices[i - 1].x, ceiling_y, vertices[i - 1].y),
            glm::vec3(vertices[i].x, ceiling_y, vertices[i].y),
        };
        glm::vec3 face_normal = glm::normalize(glm::cross(triangle_vertices[1] - triangle_vertices[0], triangle_vertices[2] - triangle_vertices[0]));
        for (unsigned int j = 0; j < 3; j++) {
            vertex_data.push_back({
                .position = triangle_vertices[j],
                .normal = face_normal,
                .texture_index = 0,
                .texture_coordinates = glm::vec2(
                        ((triangle_vertices[j].x - aabb_top_left.x) / ceiling_scale.x) * ceiling_scale.x,
                        (std::fabs(triangle_vertices[j].z - aabb_bot_right.y) / ceiling_scale.y) * ceiling_scale.y)
            });
        }

        for (unsigned int j = 0; j < 3; j++) {
            triangle_vertices[j].y = floor_y;
        }
        face_normal = glm::normalize(glm::cross(triangle_vertices[2] - triangle_vertices[0], triangle_vertices[1] - triangle_vertices[0]));
        for (unsigned int j = 0; j < 3; j++) {
            vertex_data.push_back({
                .position = triangle_vertices[j],
                .normal = face_normal,
                .texture_index = 0,
                .texture_coordinates = glm::vec2(
                        ((triangle_vertices[j].x - aabb_top_left.x) / ceiling_scale.x) * ceiling_scale.x,
                        (std::fabs(triangle_vertices[j].z - aabb_bot_right.y) / ceiling_scale.y) * ceiling_scale.y)
            });
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(VertexData), &vertex_data[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(VertexData), (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)((6 * sizeof(float)) + sizeof(unsigned int)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vertex_data_size = vertex_data.size();
    vertex_data.clear();
}

void Sector::render(unsigned int shader) {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_data_size);
    glBindVertexArray(0);
}

Frustum::Frustum(const glm::mat4& projection_view_transpose) {
    plane[0] = glm::vec4(projection_view_transpose[3] + projection_view_transpose[0]); // left
    plane[1] = glm::vec4(projection_view_transpose[3] - projection_view_transpose[0]); // right
    plane[2] = glm::vec4(projection_view_transpose[3] + projection_view_transpose[1]); // bottom
    plane[3] = glm::vec4(projection_view_transpose[3] - projection_view_transpose[1]); // top
    plane[4] = glm::vec4(projection_view_transpose[3] + projection_view_transpose[2]); // near
    plane[5] = glm::vec4(projection_view_transpose[3] - projection_view_transpose[2]); // far
}

bool Frustum::is_inside(const Sector& sector) const {
    for (unsigned int plane_index = 0; plane_index < 6; plane_index++) {
        bool all_inside_plane_halfspace = true;
        for (unsigned int aabb_index = 0; aabb_index < 8; aabb_index++) {
            if (glm::dot(sector.aabb[aabb_index], plane[plane_index]) >= 0.0f) {
                all_inside_plane_halfspace = false;
                break;
            }
        }
        if (all_inside_plane_halfspace) {
            return false;
        }
    }

    return true;
}

bool Level::init() {
    // load texture array
    glGenTextures(1, &texture_array);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 64, 64, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    const char* paths[2] = { "./res/texture/BRICK_1A.png", "./res/texture/CONSOLE_1B.png" };
    for (unsigned int i = 0; i < 2; i++) {
        int width, height, num_channels;
        unsigned char* data = stbi_load(paths[i], &width, &height, &num_channels, 0);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 64, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    // compile texture shader
    bool success = shader_compile(&texture_shader, "./shader/texture_vertex.glsl", "./shader/texture_fragment.glsl");
    if (!success) {
        return false;
    }
    glUseProgram(texture_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_array);
    glUniform1i(glGetUniformLocation(texture_shader, "texture_array"), 0);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);

    Sector a;
    a.add_vertex(glm::vec2(-3.0f, -1.0f), true);
    a.add_vertex(glm::vec2(0.0f, -5.0f), false);
    a.add_vertex(glm::vec2(3.0f, -1.0f), true);
    a.add_vertex(glm::vec2(3.0f, 5.0f), true);
    a.add_vertex(glm::vec2(-3.0f, 5.0f), true);
    a.floor_y = 0.0f;
    a.ceiling_y = 3.0f;
    sectors.push_back(a);

    Sector b;
    b.add_vertex(glm::vec2(3.0f, -9.0f), true);
    b.add_vertex(glm::vec2(6.0f, -5.0f), true);
    b.add_vertex(glm::vec2(3.0f, -1.0f), false);
    b.add_vertex(glm::vec2(0.0f, -5.0f), true);
    b.floor_y = 0.0f;
    b.ceiling_y = 3.0f;
    sectors.push_back(b);

    for (unsigned int i = 0; i < sectors.size(); i++) {
        sectors[i].init_buffers();
    }

    return true;
}


void Level::update(float delta) {
    player.update(delta);

    // check collisions and move player
    if (glm::length(player.velocity) != 0.0f) {
        glm::vec3 actual_velocity = player.velocity * delta;

        // check if within sector AABB
        std::vector<Sector*> nearby_sectors;
        for (unsigned int i = 0; i < sectors.size(); i++) {
            Sector* sector = &sectors[i];
            float padding = 1.0f;
            if (player.position.x < sector->aabb_top_left.x - padding || player.position.x > sector->aabb_bot_right.x + padding ||
                player.position.y < sector->floor_y || player.position.y > sector->ceiling_y ||
                player.position.z < sector->aabb_top_left.y - padding || player.position.z > sector->aabb_bot_right.y + padding) {
                continue;
            }
            nearby_sectors.push_back(sector);
        }

        // check floor / ceiling collisions
        glm::vec2 origin2d = glm::vec2(player.position.x, player.position.z);
        for (Sector* sector : nearby_sectors) {
            glm::vec2 raycast_start = sector->aabb_top_left - glm::vec2(1.0f, 1.0f);
            glm::vec2 raycast_direction = origin2d - raycast_start;
            unsigned int hits = 0;
            for (unsigned int wall = 0; wall < sector->vertices.size(); wall++) {
                float raycast_result = raycast(raycast_start, raycast_direction, sector->vertices[wall], sector->vertices[(wall + 1) % sector->vertices.size()] - sector->vertices[wall]);
                if (raycast_result != -1.0f) {
                    hits++;
                }
            }

            if (hits % 2 == 0) {
                continue;
            }

            float predicted_y = player.position.y + actual_velocity.y;
            if (predicted_y + 0.5f >= sector->ceiling_y) {
                glm::vec3 velocity_in_ceiling_normal_direction = glm::vec3(0.0f, -1.0f, 0.0f) * glm::dot(actual_velocity, glm::vec3(0.0f, -1.0f, 0.0f));
                actual_velocity -= velocity_in_ceiling_normal_direction;
            } else if (predicted_y - 0.5f <= sector->floor_y) {
                glm::vec3 velocity_in_floor_normal_direction = glm::vec3(0.0f, 1.0f, 0.0f) * glm::dot(actual_velocity, glm::vec3(0.0f, 1.0f, 0.0f));
                actual_velocity -= velocity_in_floor_normal_direction;
            }
        }

        // check wall collisions
        bool collided = true;
        unsigned int attempts = 0;
        while (collided && attempts < 5) {
            collided = false;
            attempts++;

            for (Sector* sector : nearby_sectors) {
                for (unsigned int wall = 0; wall < sector->vertices.size(); wall++) {
                    if (!sector->walls[wall].exists) {
                        continue;
                    }

                    glm::vec2 velocity2d = glm::vec2(actual_velocity.x, actual_velocity.z);
                    glm::vec2 predicted_origin2d = origin2d + velocity2d;

                    glm::vec2 wallv = sector->vertices[(wall + 1) % sector->vertices.size()] - sector->vertices[wall];
                    glm::vec2 f = sector->vertices[wall] - predicted_origin2d;
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

                    glm::vec3 velocity_in_wall_normal_direction = sector->walls[wall].normal * glm::dot(actual_velocity, sector->walls[wall].normal);
                    actual_velocity = actual_velocity - velocity_in_wall_normal_direction;
                    collided = true;
                }
            }
        }

        if (attempts == 5) {
            actual_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        player.position += actual_velocity;
    }
}

void Level::render() {
    glUseProgram(texture_shader);

    // view / projection transformations
    glm::mat4 view;
    view = glm::lookAt(player.position, player.position - glm::vec3(player.basis[2]), glm::vec3(player.basis[1]));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(texture_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(texture_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));

    glm::vec3 light_pos = glm::vec3(1.0f, 1.5f, -2.0f);
    glUniform3fv(glGetUniformLocation(texture_shader, "light_pos"), 1, glm::value_ptr(light_pos));
    glUniform3fv(glGetUniformLocation(texture_shader, "view_pos"), 1, glm::value_ptr(player.position));

    glm::mat4 projection_view_transpose = glm::transpose(projection * view);
    Frustum frustum = Frustum(projection_view_transpose);
    for (unsigned int i = 0; i < sectors.size(); i++) {
        if (!frustum.is_inside(sectors[i])) {
            continue;
        }

        sectors[i].render(texture_shader);
    }
}
