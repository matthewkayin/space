#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};

struct Mesh {
    unsigned int vao, vbo, ebo;
    unsigned int num_indices;

    // std::vector<Vertex> vertices;
    // std::vector<unsigned int> indices;
    // std::vector<unsigned int> textures;

    Mesh(const float* vertices, unsigned int vertices_size, const unsigned int* indices, unsigned int indices_size);
    void init();
    void draw(unsigned int shader);
};
