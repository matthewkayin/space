#version 410 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texture_coordinate;

out vec2 texture_coordinate;
out vec3 frag_pos;

uniform ivec2 extents;
uniform ivec2 screen_size;

uniform vec3 view_pos;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    frag_pos = vec3(model * vec4((a_pos.x * extents.x) / (screen_size.x / 2), (a_pos.y * extents.y) / (screen_size.y / 2), 0.0, 1.0));
    gl_Position = projection * view * vec4(frag_pos, 1.0);
    // frag_pos = vec3(model * vec4(a_pos.x, a_pos.y, 0.0, 1.0));
    texture_coordinate = a_texture_coordinate;
}
