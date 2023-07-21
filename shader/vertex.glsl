#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 texture_coordinates;

out vec4 frag_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 light_pos;
uniform vec3 view_pos;

uniform sampler2D atlas;

void main() {
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    vec3 frag_pos = vec3(model * vec4(a_pos, 1.0));
    vec3 normal = mat3(transpose(inverse(model))) * a_normal;

    vec3 ambient = 0.1 * light_color;

    vec3 light_direction = normalize(light_pos - frag_pos);
    vec3 diffuse = max(dot(normal, light_direction), 0.0) * light_color;

    vec3 view_direction = normalize(view_pos - frag_pos);
    vec3 reflect_direction = reflect(-light_direction, normal);
    vec3 specular = 0.5 * pow(max(dot(view_direction, reflect_direction), 0.0), 32) * light_color;

    float frag_distance = length(light_pos - frag_pos);
    float light_constant = 1.0;
    float light_linear = 0.022;
    float light_quadratic = 0.0019;
    float attenuation = 1.0 / (light_constant + (light_linear * frag_distance) + (light_quadratic * frag_distance * frag_distance));

    vec3 sampled = texture(atlas, texture_coordinates).rgb;
    frag_color = vec4((ambient + diffuse + specular) * attenuation * sampled, 1.0);
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
