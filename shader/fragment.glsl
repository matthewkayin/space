#version 410 core

out vec4 FragColor;

flat in int texture_index;
in vec2 texture_coordinate;

uniform sampler2D atlas;
uniform vec2 atlas_step;
uniform ivec2 atlas_count;

void main() {
    vec2 texture_position_base = vec2(texture_index % atlas_count.x, int(float(texture_index) / atlas_count.y));
    vec2 texture_position_offset = vec2(fract(texture_coordinate.x), fract(texture_coordinate.y));
    vec2 texture_position = texture_position_base + texture_position_offset;
    FragColor = texture(atlas, vec2(texture_position.x * atlas_step.x, texture_position.y * atlas_step.y));
}
