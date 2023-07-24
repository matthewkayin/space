#include "level.hpp"

#include "shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <cstdio>

const int TEXTURE_SIZE = 64;

Sector::Sector() {
    vertices.push_back(glm::vec2(-3.0f, -1.0f));
    vertices.push_back(glm::vec2(0.0f, -5.0f));
    vertices.push_back(glm::vec2(3.0f, -1.0f));
    vertices.push_back(glm::vec2(3.0f, 5.0f));
    vertices.push_back(glm::vec2(-3.0f, 5.0f));
    floor_y = 0.0f;
    ceiling_y = 3.0f;
}

void Sector::init_buffers() {
    // walls
    for (unsigned int i = 0; i < vertices.size(); i++) {
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
                wall_normals.push_back(face_normal);
            }
        }
    }

    // ceiling and floor
    aabb_top_left = vertices[0];
    aabb_bot_right = vertices[0];
    for (unsigned int i = 1; i < vertices.size(); i++) {
        aabb_top_left.x = std::min(aabb_top_left.x, vertices[i].x);
        aabb_top_left.y = std::min(aabb_top_left.y, vertices[i].y);
        aabb_bot_right.x = std::max(aabb_bot_right.x, vertices[i].x);
        aabb_bot_right.y = std::max(aabb_bot_right.y, vertices[i].y);
    }
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

glm::vec3 Sector::get_collision_normal(const glm::vec3& point) const {
    // first check if within bounding box
    if (point.x < aabb_top_left.x || point.y > aabb_bot_right.x ||
        point.y < floor_y || point.y > ceiling_y ||
        point.z < aabb_top_left.y || point.z > aabb_bot_right.y) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // next check if within polygon
    glm::vec2 raycast_start = aabb_top_left - glm::vec2(1.0f, 1.0f);
    glm::vec2 point2d = glm::vec2(point.x, point.z);
    glm::vec2 raycast_direction = point2d - raycast_start;

    unsigned int hits = 0;
    for (unsigned int wall = 0; wall < vertices.size(); wall++) {
        float raycast_result = raycast(raycast_start, raycast_direction, vertices[wall], vertices[(wall + 1) % vertices.size()] - vertices[wall]);
        if (raycast_result != -1.0f && raycast_result <= glm::length(raycast_direction)) {
            hits++;
        }
    }

    if (hits % 2 == 0) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    glm::vec3 collision_normal = glm::vec3(0.0f, 0.0f, 0.0f);
    float player_radius = 0.5f;
    float player_radius_squared = player_radius * player_radius;

    // check for floor/ceiling collisions
    // assumes they won't collide into both on the same frame
    if (std::fabs(point.y - floor_y) < player_radius) {
        collision_normal += glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (std::fabs(point.y - ceiling_y) < player_radius) {
        collision_normal += glm::vec3(0.0f, -1.0f, 0.0f);
    }

    for (unsigned int wall = 0; wall < vertices.size(); wall++) {
        glm::vec2 wallv = vertices[(wall + 1) % vertices.size()] - vertices[wall];
        glm::vec2 f = vertices[wall] - point2d;
        float a = glm::dot(wallv, wallv);
        float b = 2 * glm::dot(f, wallv);
        float c = glm::dot(f, f) - player_radius_squared;
        float discriminant = (b * b) - (4 * a * c);

        if (discriminant < 0.0f) {
            continue;
        }

        collision_normal += wall_normals[wall];
    }

    if (collision_normal.x != 0.0f || collision_normal.y != 0.0f || collision_normal.z != 0.0f) {
        collision_normal = glm::normalize(collision_normal);
    }

    return collision_normal;
}


void Sector::render(unsigned int shader) {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_data_size);
    glBindVertexArray(0);
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

    sector.init_buffers();

    return true;
}


void Level::update(float delta) {
    player.update(delta);
    player.position += player.velocity * delta;

    // check collisions
    if (glm::length(player.velocity) != 0.0f) {
        glm::vec3 collision_normal = sector.get_collision_normal(player.position);
        glm::vec3 velocity_in_wall_normal_direction = collision_normal * glm::dot(player.velocity * delta, collision_normal);
        player.position -= velocity_in_wall_normal_direction;
    }
}

void Level::render() {
    // glUseProgram(texture_shader);
    sector.render(texture_shader);
}
