#include "level.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#include <cstdio>

const int TEXTURE_SIZE = 64;

void Level::load_texture_atlas() {
    glGenTextures(1, &texture_atlas);
    glBindTexture(GL_TEXTURE_2D, texture_atlas);
    int width, height, num_channels;
    unsigned char* data = stbi_load("./res/textures.png", &width, &height, &num_channels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        texture_atlas_count = glm::ivec2(width / TEXTURE_SIZE, height / TEXTURE_SIZE);
        texture_coordinate_step = glm::vec2(1.0f / (float)texture_atlas_count.x, 1.0f / (float)texture_atlas_count.y);
    } else {
        printf("Failed to load texture atlas\n");
    }
    stbi_image_free(data);
}

glm::vec2 Level::get_texture_coordinates(unsigned int texture_index) {
    glm::vec2 texture_index_2d = glm::vec2(texture_index % texture_atlas_count.x, (int)(texture_index / (float)texture_atlas_count.x));
    if (texture_index_2d.x >= texture_atlas_count.x || texture_index_2d.y >= texture_atlas_count.y) {
        texture_index_2d = glm::vec2(0.0f, 0.0f);
    }

    return texture_index_2d;
}

void Level::init() {
    load_texture_atlas();

    std::vector<glm::vec2> vertices;
    vertices.push_back(glm::vec2(-3.0f, -1.0f));
    vertices.push_back(glm::vec2(0.0f, -5.0f));
    vertices.push_back(glm::vec2(3.0f, -1.0f));
    vertices.push_back(glm::vec2(3.0f, 5.0f));
    vertices.push_back(glm::vec2(-3.0f, 5.0f));
    float floor_y = 0.0f;
    float ceiling_y = 3.0f;

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

        glm::vec2 texture_index_origin = get_texture_coordinates(0);
        glm::vec2 texture_coordinate_offsets[6] = {
            glm::vec2(0.0f, 2.0f),
            glm::vec2(2.0f, 2.0f),
            glm::vec2(0.0f, 0.0f),

            glm::vec2(2.0f, 2.0f),
            glm::vec2(2.0f, 0.0f),
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
                    .texture_coordinates = texture_index_origin + (texture_coordinate_offsets[base_index + j])
                });
            }
        }
    }

    // ceiling and floor
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
                .texture_coordinates = glm::vec2(0.0f, 0.0f)
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
                .texture_coordinates = glm::vec2(0.0f, 0.0f)
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
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(VertexData), (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)((6 * sizeof(float) + sizeof(int))));

    glBindVertexArray(0);
}

void Level::render(unsigned int shader) {
    /*glm::vec3 white = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(glGetUniformLocation(shader, "object_color"), 1, glm::value_ptr(white));*/
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_atlas);
    glUniform1i(glGetUniformLocation(shader, "atlas"), 0);
    glm::vec2 atlas_step = glm::vec2(0.5f, 1.0f);
    glUniform2fv(glGetUniformLocation(shader, "atlas_step"), 1, glm::value_ptr(atlas_step));
    glm::ivec2 atlas_count = glm::ivec2(2, 1);
    glUniform2iv(glGetUniformLocation(shader, "atlas_count"), 1, glm::value_ptr(atlas_count));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_data.size());
    glBindVertexArray(0);
}
