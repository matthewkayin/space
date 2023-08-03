#include "shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <cstdio>
#include <sstream>

unsigned int text_shader;
unsigned int texture_shader;
unsigned int gun_shader;
unsigned int screen_shader;
unsigned int ui_shader;

bool shader_compile(unsigned int* id, const char* vertex_path, const char* fragment_path);

bool shader_compile_all() {
    if (!shader_compile(&text_shader, "./shader/text_vertex.glsl", "./shader/text_fragment.glsl")) {
        return false;
    }
    if (!shader_compile(&texture_shader, "./shader/texture_vertex.glsl", "./shader/texture_fragment.glsl")) {
        return false;
    }
    if (!shader_compile(&gun_shader, "./shader/gun_vertex.glsl", "./shader/gun_fragment.glsl")) {
        return false;
    }
    if (!shader_compile(&screen_shader, "./shader/screen_vertex.glsl", "./shader/screen_fragment.glsl")) {
        return false;
    }
    if (!shader_compile(&ui_shader, "./shader/ui_vertex.glsl", "./shader/ui_fragment.glsl")) {
        return false;
    }

    return true;
}

bool shader_compile(unsigned int* id, const char* vertex_path, const char* fragment_path) {
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vertex_shader_file;
    std::ifstream fragment_shader_file;

    // ensure ifstream objects can throw exceptions
    vertex_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragment_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vertex_shader_file.open(vertex_path);
        fragment_shader_file.open(fragment_path);
        std::stringstream vertex_shader_stream;
        std::stringstream fragment_shader_stream;

        vertex_shader_stream << vertex_shader_file.rdbuf();
        fragment_shader_stream << fragment_shader_file.rdbuf();

        vertex_shader_file.close();
        fragment_shader_file.close();

        vertex_code = vertex_shader_stream.str();
        fragment_code = fragment_shader_stream.str();
    } catch (std::exception& e) {
        printf("Error: shader file (%s, %s) not successfully read\n", vertex_path, fragment_path);
        return false;
    }

    const char* vertex_shader_code = vertex_code.c_str();
    const char* fragment_shader_code = fragment_code.c_str();

    unsigned int vertex;
    unsigned int fragment;
    int success;
    char info_log[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertex_shader_code, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, info_log);
        printf("Error: vertex shader %s compilation failed\n%s\n", vertex_path, info_log);
        return false;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragment_shader_code, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, info_log);
        printf("Error: fragment shader %s compilation failed\n%s\n", fragment_path, info_log);
        return false;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, info_log);
        printf("Error linking shader program (%s %s) \n%s\n", vertex_path, fragment_path, info_log);
        return false;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    *id = program;
    return true;
}
