#pragma once

#include <glm/glm.hpp>
#include <vector>

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coordinates;
};

struct Level {
    unsigned int texture_array;

    unsigned int vao, vbo;
    std::vector<VertexData> vertex_data;

    void init();
    void render(unsigned int shader);
};
