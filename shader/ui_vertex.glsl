#version 410 core
layout (location = 0) in vec2 a_pos;

uniform ivec2 screen_size;
uniform ivec2 extents;
uniform ivec2 position;

void main() {
    ivec2 vertex_position = position + ivec2(extents.x * a_pos.x, extents.y * a_pos.y);
    gl_Position = vec4(vertex_position.x / (screen_size.x / 2.0), vertex_position.y / (screen_size.y / 2.0), 0.0, 1.0);
}
