#version 410 core

in vec2 tex_coords;
in vec2 vertex_pos;
out vec4 color;

uniform vec2 texture_size;
uniform sampler2D u_texture;
uniform vec3 text_color;
uniform int glyph_size;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(u_texture, vec2(((tex_coords.x + vertex_pos.x) * glyph_size) / texture_size.x, 1 - (((tex_coords.y + 1 - vertex_pos.y) * glyph_size) / texture_size.y))).r);
    color = vec4(text_color, 1.0) * sampled;
}
