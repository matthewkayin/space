#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct Mesh {
    unsigned int vao, vbo, ebo;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // std::vector<unsigned int> textures;

    Mesh(const float* vertex_positions, unsigned int vertex_positions_size, const unsigned int* indices, unsigned int indices_size);
    void draw(unsigned int shader);
};
