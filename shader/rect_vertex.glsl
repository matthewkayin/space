#version 410 core
layout (location = 0) in vec2 vertex;

uniform ivec2 screen_size;
uniform ivec2 position;
uniform ivec2 extents;

void main() {
    vec2 clip_position = position + vec2(extents.x * vertex.x, extents.y * vertex.y);
    clip_position.x = clip_position.x / (screen_size.x / 2.0);
    clip_position.y = -clip_position.y / (screen_size.y / 2.0);
    gl_Position = vec4(vec2(-1.0, 1.0) + clip_position, 0.0, 1.0);
}
