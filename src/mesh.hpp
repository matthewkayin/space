#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coord;
};

struct Mesh {
    unsigned int vao, vbo, ebo;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 color;

    Mesh(const float* vertex_positions, unsigned int vertex_positions_size, const unsigned int* indices, unsigned int indices_size);
    void draw(unsigned int shader);
};
