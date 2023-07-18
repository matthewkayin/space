#version 410 core
layout (location = 0) in vec2 vertex;
out vec2 tex_coords;
out vec2 vertex_pos;
uniform int glyph_size;
uniform mat4 projection;
uniform vec2 glyph_coords;
uniform vec2 texture_coords;

void main() {
    gl_Position = projection * vec4(glyph_coords + (vertex * glyph_size), 0.0, 1.0);
    tex_coords = texture_coords;
    vertex_pos = vertex;
}
