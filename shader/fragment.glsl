#version 410 core

out vec4 FragColor;

in vec2 texture_coordinate;

uniform sampler2DArray texture_array;

void main() {
    // FragColor = textureGrad(atlas, vec2(texture_position.x * atlas_step.x, texture_position.y * atlas_step.y), dFdx(texture_coordinate), dFdy(texture_coordinate));
    FragColor = vec4(texture(texture_array, vec3(texture_coordinate.x, texture_coordinate.y, 0)));
}
