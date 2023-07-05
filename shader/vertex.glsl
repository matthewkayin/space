#version 410 core
layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;

// out vec2 tex_coords;
out vec3 normal;
out vec4 frag_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 light_pos;
uniform vec3 view_pos;
uniform vec3 object_color;

void main() {
    vec4 light_color = vec4(1.0, 1.0, 1.0, 1.0);

    vec3 frag_pos = vec3(model * vec4(a_pos, 1.0));
    normal = normalize(mat3(transpose(inverse(view * model))) * a_normal);

    vec4 ambient = 0.25 * light_color;

    vec3 norm = normalize(normal);
    vec3 light_direction = normalize(light_pos - frag_pos);
    vec4 diffuse = max(dot(norm, light_direction), 0.0) * light_color;

    vec3 view_direction = normalize(view_pos - frag_pos);
    vec3 reflect_direction = reflect(-light_direction, norm);
    vec4 specular = 0.5 * pow(max(dot(view_direction, reflect_direction), 0.0), 32) * light_color;

    frag_color = (ambient + diffuse + specular) * vec4(object_color, 1.0);
    gl_Position = projection * view * model * vec4(a_pos, 1.0);
}
