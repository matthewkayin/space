#pragma once

struct Font {
    float height;
    unsigned int* textures;
    unsigned int list_base;
    Font(const char* path, unsigned int height);
    ~Font();
    void render_text(float x, float y, const char* fmt, ...);
};
