#version 410 core

out vec4 FragColor;

// in vec2 tex_coords;
in vec4 frag_color;

// uniform sampler2D texture_diffuse1;

void main() {
    FragColor = frag_color;
}
