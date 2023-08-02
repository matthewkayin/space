#pragma once

#include <glm/glm.hpp>
#include <string>

struct Font {
    unsigned int atlas;
    unsigned int glyph_size;
    glm::vec2 atlas_size;

    Font() { }
    Font(const char* path, unsigned int size);
    void render_text(std::string text, float x, float y, glm::vec3 color);
};

extern Font font_hack_10pt;

void font_init();
