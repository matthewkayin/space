#pragma once

#include <glm/glm.hpp>
#include <vector>

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    int texture_index;
    glm::vec2 texture_coordinates;
};

struct Level {
    unsigned int texture_atlas;
    glm::ivec2 texture_atlas_count;
    glm::vec2 texture_coordinate_step;

    unsigned int vao, vbo;
    std::vector<VertexData> vertex_data;

    void load_texture_atlas();
    glm::vec2 get_texture_coordinates(unsigned int texture_index);
    void init();
    void render(unsigned int shader);
};
