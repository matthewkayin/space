#include "edit.hpp"

#include "globals.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

bool Edit::init() {
    // compile shaders
    if (!shader_compile(&rect_shader, "./shader/rect_vertex.glsl", "./shader/edit_fragment.glsl")) {
        return false;
    }
    glUseProgram(rect_shader);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCREEN_WIDTH), 0.0f, static_cast<float>(SCREEN_HEIGHT));
    glUniformMatrix4fv(glGetUniformLocation(rect_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform2iv(glGetUniformLocation(rect_shader, "screen_size"), 1, glm::value_ptr(screen_size));

    // init vertex data
    float rect_vertices[12] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        -1.0f, 1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f
    };

    glGenVertexArrays(1, &rect_vao);
    glGenBuffers(1, &rect_vbo);
    glBindVertexArray(rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect_vertices), &rect_vertices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

void Edit::render() {
    glUseProgram(rect_shader);
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 0.0f);
    glUniform3fv(glGetUniformLocation(rect_shader, "u_color"), 1, glm::value_ptr(color));

    glm::ivec2 points[] = {
        glm::ivec2(10, 10),
        glm::ivec2(20, 10),
        glm::ivec2(5, 20)
    };

    glBindVertexArray(rect_vao);
    glm::ivec2 size = glm::ivec2(2, 2);
    glUniform2iv(glGetUniformLocation(rect_shader, "extents"), 1, glm::value_ptr(size));
    for (unsigned int i = 0; i < 3; i++) {
        glUniform2iv(glGetUniformLocation(rect_shader, "position"), 1, glm::value_ptr(points[i]));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
