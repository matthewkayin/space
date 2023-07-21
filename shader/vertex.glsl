#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in int a_texture_index;
layout (location = 3) in vec2 a_texture_coordinate;

flat out int texture_index;
out vec2 texture_coordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 light_pos;
uniform vec3 view_pos;

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

    texture_index = a_texture_index;
    texture_coordinate = a_texture_coordinate;
    // frag_color = texture(atlas, a_tex_coord);
    // frag_color = vec4((ambient + diffuse + specular) * attenuation * sampled, 1.0);
    // frag_color = vec4(sampled, 1.0);
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
