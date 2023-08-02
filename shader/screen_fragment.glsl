#version 410 core

out vec4 frag_color;

in vec2 texture_coordinate;

uniform sampler2D screen_texture;

void main() {
    frag_color = texture(screen_texture, texture_coordinate);
}
