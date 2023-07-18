#include "font.hpp"

#include "globals.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const int ATLAS_FIRST_CHAR = 32;

unsigned int glyph_vao;
unsigned int text_shader;

Font font_hack_10pt;

void font_init() {
    // buffer glyph vertex data
    unsigned int glyph_vbo;

    float glyph_vertices[12] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
    };

    glGenVertexArrays(1, &glyph_vao);
    glGenBuffers(1, &glyph_vbo);
    glBindVertexArray(glyph_vao);
    glBindBuffer(GL_ARRAY_BUFFER, glyph_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glyph_vertices), glyph_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // compile text shader
    text_shader = shader_compile("./shader/text_vertex.glsl", "./shader/text_fragment.glsl");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCREEN_WIDTH), 0.0f, static_cast<float>(SCREEN_HEIGHT));
    glUseProgram(text_shader);
    glUniformMatrix4fv(glGetUniformLocation(text_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // load fonts
    font_hack_10pt = Font("./hack_10pt.bmp", 10);
}

Font::Font(const char* path, unsigned int size) {
    glGenTextures(1, &atlas);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int width, height, num_channels;
    unsigned char* data = stbi_load(path, &width, &height, &num_channels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        atlas_size = glm::vec2(width, height);
        glyph_size = size;
    } else {
        printf("Failed to load hack 10pt\n");
    }
    stbi_image_free(data);
}

void Font::render_text(std::string text, float x, float y, glm::vec3 color) {
    glUseProgram(text_shader);

    glActiveTexture(GL_TEXTURE0);

    glUniform3f(glGetUniformLocation(text_shader, "text_color"), color.x, color.y, color.z);
    glUniform1i(glGetUniformLocation(text_shader, "u_texture"), 0);
    glUniform2fv(glGetUniformLocation(text_shader, "texture_size"), 1, glm::value_ptr(atlas_size));
    glUniform1i(glGetUniformLocation(text_shader, "glyph_size"), glyph_size);

    glBindTexture(GL_TEXTURE_2D, atlas);
    glBindVertexArray(glyph_vao);

    int glyphs_per_row = (int)(atlas_size.x / glyph_size);

    for (unsigned int i = 0; i < text.length(); i++) {
        int char_index = ((int)text[i]) - ATLAS_FIRST_CHAR;
        glm::vec2 glyph_coords = glm::vec2(x + (glyph_size * i), SCREEN_HEIGHT - glyph_size - y);
        glm::vec2 glyph_texture_coords = glm::vec2(char_index % glyphs_per_row, (int)(char_index / (float)glyphs_per_row));

        glUniform2fv(glGetUniformLocation(text_shader, "glyph_coords"), 1, glm::value_ptr(glyph_coords));
        glUniform2fv(glGetUniformLocation(text_shader, "texture_coords"), 1, glm::value_ptr(glyph_texture_coords));

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
