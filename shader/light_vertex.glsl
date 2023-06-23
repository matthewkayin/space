#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;

// out vec2 tex_coords;
out vec3 normal;
out vec4 frag_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    frag_color = vec4(1.0, 1.0, 1.0, 1.0);
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
