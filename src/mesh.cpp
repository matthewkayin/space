#include "mesh.hpp"

#include <glad/glad.h>

Mesh::Mesh(const float* vertex_positions, unsigned int vertex_positions_size, const unsigned int* indices, unsigned int indices_size) {
    // Per vertex shading
    for (unsigned int i = 0; i < (vertex_positions_size / sizeof(float)) / 3; i++) {
        glm::vec3 position = glm::vec3(vertex_positions[i * 3], vertex_positions[(i * 3) + 1], vertex_positions[(i * 3) + 2]);
        Vertex vertex = (Vertex) {
            .position = position,
            .normal = glm::vec3(0.0f, 0.0f, 0.0f),
        };
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < indices_size / sizeof(unsigned int); i++) {
        this->indices.push_back(indices[i]);
    }

    for (unsigned int face = 0; face < this->indices.size() / 3; face++) {
        unsigned int face_indices[3];
        for (int i = 0; i < 3; i++) {
            face_indices[i] = indices[(face * 3) + i];
        }

        glm::vec3 face_normal = glm::cross(vertices[face_indices[2]].position - vertices[face_indices[0]].position, vertices[face_indices[1]].position - vertices[face_indices[0]].position);

        for (int i = 0; i < 3; i++) {
            vertices[face_indices[i]].normal += face_normal;
        }
    }

    for (unsigned int i = 0; i < vertices.size(); i++) {
        vertices[i].normal = glm::normalize(vertices[i].normal);
        printf("%f %f %f / %f %f %f \n", vertices[i].position.x, vertices[i].position.y, vertices[i].position.z, vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z);
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

    /*

    // vertex texture coordinates
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
    */

    glBindVertexArray(0);
}

void Mesh::draw(unsigned int shader) {
    /*for (unsigned int i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glUniform1i(glGetUniformLocation(shader, ("texture" + std::to_string(i + 1)).c_str()), i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
    }*/

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // glActiveTexture(GL_TEXTURE0);
}
