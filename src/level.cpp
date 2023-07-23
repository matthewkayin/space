#include "level.hpp"

#include "shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <cstdio>

const int TEXTURE_SIZE = 64;

CollisionTriangle::CollisionTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 p_normal) {
    normal = p_normal;

    float length01 = glm::length(v1 - v0);
    float length12 = glm::length(v2 - v1);
    float length20 = glm::length(v0 - v2);

    if (length01 >= length12 && length01 >= length20) {
        a = v0;
        b = v1;
        c = v2;
    } else if (length12 >= length20) {
        a = v1;
        b = v2;
        c = v0;
    } else {
        a = v2;
        b = v0;
        c = v1;
    }

    unit_u = glm::normalize(b - a);
    unit_v = glm::normalize((c - a) - (unit_u * glm::dot(unit_u, c - a)));
    w = glm::length(b - a);
    g = glm::dot(unit_u, c - a);
    h = glm::dot(unit_v, c - a);
}

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
            collision_triangles.push_back(CollisionTriangle(
                wall_vertices[base_index + 0],
                wall_vertices[base_index + 1],
                wall_vertices[base_index + 2],
                face_normal
            ));
        }
    }

    // ceiling and floor
    float ceiling_texture_top = vertices[0].y;
    float ceiling_texture_left = vertices[0].x;
    float ceiling_texture_bot = vertices[0].y;
    float ceiling_texture_right = vertices[0].x;
    for (unsigned int i = 1; i < vertices.size(); i++) {
        ceiling_texture_left = std::min(ceiling_texture_left, vertices[i].x);
        ceiling_texture_top = std::min(ceiling_texture_top, vertices[i].y);
        ceiling_texture_right = std::max(ceiling_texture_right, vertices[i].x);
        ceiling_texture_bot = std::max(ceiling_texture_bot, vertices[i].y);
    }
    glm::vec2 ceiling_scale = glm::vec2(std::fabs(ceiling_texture_right - ceiling_texture_left), std::fabs(ceiling_texture_top - ceiling_texture_bot));
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
                        ((triangle_vertices[j].x - ceiling_texture_left) / ceiling_scale.x) * ceiling_scale.x,
                        (std::fabs(triangle_vertices[j].z - ceiling_texture_bot) / ceiling_scale.y) * ceiling_scale.y)
            });
        }
        collision_triangles.push_back(CollisionTriangle(
            triangle_vertices[0],
            triangle_vertices[1],
            triangle_vertices[2],
            face_normal
        ));

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
                        ((triangle_vertices[j].x - ceiling_texture_left) / ceiling_scale.x) * ceiling_scale.x,
                        (std::fabs(triangle_vertices[j].z - ceiling_texture_bot) / ceiling_scale.y) * ceiling_scale.y)
            });
        }
        collision_triangles.push_back(CollisionTriangle(
            triangle_vertices[0],
            triangle_vertices[1],
            triangle_vertices[2],
            face_normal
        ));
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

float Sector::collision_check(const glm::vec3 point, const glm::vec3 velocity) const {
    // TODO check point against an AABB to see if it's within the sector

    glm::vec3 direction = glm::normalize(velocity);

    for (const CollisionTriangle& triangle : collision_triangles) {
        // check that direction is not perpendicular to triangle normal
        float d_dot_n = glm::dot(direction, triangle.normal);
        if (d_dot_n == 0.0f) {
            continue;
        }

        // check whether ray intersects with triangle plane
        float nd = glm::dot(triangle.normal, triangle.a - point);
        float t = (nd - glm::dot(point, triangle.normal)) / d_dot_n;
        if (t < 0) {
            continue;
        }

        float u = glm::dot(triangle.unit_u, point - triangle.a);
        float v = glm::dot(triangle.unit_v, point - triangle.a);
        if (u < 0 || u > triangle.w || v < 0 || v > triangle.w ||
            (u <= triangle.g && v > u * (triangle.h / triangle.g)) ||
            (u >= triangle.g && v > triangle.w - (u * (triangle.h / (triangle.w - triangle.g))))) {
            continue;
        }

        return std::min(t, glm::length(velocity));
    }
    return glm::length(velocity);
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

    if (glm::length(player.velocity) != 0.0f) {
        float velocity_length = sector.collision_check(player.position, player.velocity * delta);
        printf("v %f\n", velocity_length);
        player.position += player.velocity * velocity_length;
    }
}

void Level::render() {
    // glUseProgram(texture_shader);
    sector.render(texture_shader);
}
