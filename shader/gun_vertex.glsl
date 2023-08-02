#version 410 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texture_coordinate;

out vec2 texture_coordinate;
out vec3 frag_pos;

uniform vec3 view_pos;
uniform mat4 projection;

void main() {
    gl_Position = vec4(a_pos.x, a_pos.y, 0.0, 1.0);
    frag_pos = view_pos + vec3(a_pos.x, a_pos.y, 0.0);
    texture_coordinate = a_texture_coordinate;
}
