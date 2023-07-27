#pragma once

struct Edit {
    unsigned int rect_shader;
    unsigned int rect_vao;
    unsigned int rect_vbo;

    bool init();
    void render();
};
