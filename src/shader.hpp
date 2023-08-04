#pragma once

#include <glm/glm.hpp>

#include <string>

extern unsigned int text_shader;
extern unsigned int texture_shader;
extern unsigned int billboard_shader;
extern unsigned int screen_shader;
extern unsigned int ui_shader;

bool shader_compile_all();
