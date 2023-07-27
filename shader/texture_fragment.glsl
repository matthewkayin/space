#version 410 core

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutoff;
    float outer_cutoff;

    float constant;
    float linear;
    float quadratic;
};

out vec4 FragColor;

flat in uint texture_index;
in vec2 texture_coordinate;
in vec3 frag_pos;
in vec3 normal;

uniform vec3 view_pos;
uniform PointLight point_lights[4];
uniform uint point_light_count;
uniform SpotLight player_flashlight;
uniform sampler2DArray texture_array;

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_direction);
vec3 calculate_spot_light(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_direction);

void main() {
    vec3 view_direction = normalize(view_pos - frag_pos);
    vec3 light_result = vec3(1.0, 1.0, 1.0) * 0.025;
    light_result += calculate_spot_light(player_flashlight, normal, frag_pos, view_direction);
    for(uint i = 0; i < point_light_count; i++) {
        light_result += calculate_point_light(point_lights[i], normal, frag_pos, view_direction);
    }

    FragColor = vec4(light_result, 1.0) * vec4(texture(texture_array, vec3(texture_coordinate.x, texture_coordinate.y, texture_index)));
}

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_direction) {
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    vec3 ambient = 0.1 * light_color;

    vec3 light_direction = normalize(light.position - frag_pos);
    vec3 diffuse = max(dot(normal, light_direction), 0.0) * light_color;

    vec3 reflect_direction = reflect(-light_direction, normal);
    vec3 specular = 0.5 * pow(max(dot(view_direction, reflect_direction), 0.0), 32) * light_color;

    float vertex_distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + (light.linear * vertex_distance) + (light.quadratic * vertex_distance * vertex_distance));

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calculate_spot_light(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_direction) {
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    vec3 ambient = 0.1 * light_color;

    vec3 light_direction = normalize(light.position - frag_pos);
    vec3 diffuse = max(dot(normal, light_direction), 0.0) * light_color;

    vec3 reflect_direction = reflect(-light_direction, normal);
    vec3 specular = 0.5 * pow(max(dot(view_direction, reflect_direction), 0.0), 32) * light_color;

    float vertex_distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + (light.linear * vertex_distance) + (light.quadratic * vertex_distance * vertex_distance));

    float theta = dot(light_direction, -light.direction);
    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    return (ambient + diffuse + specular) * attenuation * intensity;
}
