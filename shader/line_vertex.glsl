#version 410 core
layout (location = 0) in vec2 vertex;

uniform vec2 u_offset;

void main() {
    gl_Position = vec4(vertex + u_offset, 0.0, 1.0);
}
