#include "model.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <fstream>
#include <vector>

struct Face {
    unsigned int position_indices[3];
    unsigned int texture_coordinate_indices[3];
    unsigned int normal_indices[3];
};

struct ModelVertexData {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coordinates;
};

Model::Model() { }

void Model::open(std::string path) {
    std::ifstream filein(path.c_str());

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texture_coordinates;
    std::vector<glm::vec3> normals;
    std::vector<Face> faces;

    for (std::string line; std::getline(filein, line);) {
        if (line[0] == '#') {
            continue;
        }
        std::vector<std::string> words;
        while (line != "") {
            std::size_t space_index = line.find(" ");
            if (space_index == std::string::npos) {
                words.push_back(line);
                line = "";
            } else {
                words.push_back(line.substr(0, space_index));
                line = line.substr(space_index + 1);
            }
        }

        if (words[0] == "v") {
            positions.push_back(glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3])));
        } else if (words[0] == "vt") {
            texture_coordinates.push_back(glm::vec2(std::stof(words[1]), std::stof(words[2])));
        } else if (words[0] == "vn") {
            normals.push_back(glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3])));
        } else if (words[0] == "f") {
            Face face;
            for (unsigned int i = 0; i < 3; i++) {
                std::size_t slash_index = words[i + 1].find("/");
                std::size_t second_slash_index = words[i + 1].find("/", slash_index + 1);
                // subtract all indices by one because OBJ file indices start at 1 instead of 0
                face.position_indices[i] = std::stoi(words[i + 1].substr(0, slash_index)) - 1;
                face.texture_coordinate_indices[i] = std::stoi(words[i + 1].substr(slash_index + 1, second_slash_index)) - 1;
                face.normal_indices[i] = std::stoi(words[i + 1].substr(second_slash_index + 1)) - 1;
            }
            faces.push_back(face);
        }
    }

    std::vector<ModelVertexData> vertex_data;
    for (Face face : faces) {
        for (unsigned int i = 0; i < 3; i++) {
            vertex_data.push_back(ModelVertexData {
                .position = positions[face.position_indices[i]],
                .normal = normals[face.normal_indices[i]],
                .texture_coordinates = texture_coordinates[face.texture_coordinate_indices[i]]
            });
        }
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(ModelVertexData), &vertex_data[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertexData), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertexData), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertexData), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vertex_data_size = vertex_data.size();
}

void Model::render(unsigned int shader, glm::vec3 position) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_data_size);
    glBindVertexArray(0);
}
