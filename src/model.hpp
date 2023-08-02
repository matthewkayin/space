#pragma once

#include <glm/glm.hpp>
#include <string>

struct Model {
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int vertex_data_size;

    Model();
    void open(std::string path);
    void render(unsigned int shader, glm::vec3 position);
};