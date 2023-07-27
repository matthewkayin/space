#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in uint a_texture_index;
layout (location = 3) in vec2 a_texture_coordinate;

flat out uint texture_index;
out vec2 texture_coordinate;
out vec3 frag_pos;
out vec3 normal;

uniform mat4 view;
uniform mat4 projection;

void main() {
    texture_index = a_texture_index;
    texture_coordinate = a_texture_coordinate;
    frag_pos = a_pos;
    normal = a_normal;
    gl_Position = projection * view * vec4(a_pos, 1.0);
}
