#version 410 core

out vec4 FragColor;

flat in uint texture_index;
in vec2 texture_coordinate;
in vec4 lighting_color;

uniform sampler2DArray texture_array;

void main() {
    FragColor = lighting_color * vec4(texture(texture_array, vec3(texture_coordinate.x, texture_coordinate.y, texture_index)));
}
