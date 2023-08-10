#version 410 core

out vec4 frag_color;

in vec2 texture_coordinate;

uniform sampler2D screen_texture;
uniform float elapsed;
uniform float time;
uniform uint player_health = 100;
uniform uint disable_noise = 0;

const float corner_harshness = 0.5;
const float corner_ease = 4.0;
const float fade_speed = 2.0;
const float fade_amount = 16.0;

const float flash_start_time = 0.0;
const float flash_duration = 0.1;
const float fade_start_time = flash_start_time + flash_duration;
const float fade_end_time = fade_start_time + 1.5;

float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123 * elapsed);
}

void main() {
    if (time >= fade_end_time) {
        frag_color = vec4(0.0);
        return;
    }

    vec2 tuv = texture_coordinate;
    if (time > fade_start_time) {
        float interpolation = pow(cos((time - fade_start_time) + 4.8) * fade_speed, fade_amount);
        float fade = max(interpolation, 1.0);

        float xx = abs(texture_coordinate.x - 0.5) * corner_harshness;
        float yy = abs(texture_coordinate.y - 0.5) * corner_harshness * fade;
        float rr = 1.0 + pow((xx * xx) + (yy * yy), corner_ease);
        tuv = clamp(((texture_coordinate - 0.5) * rr) + 0.5, 0.0, 1.0);
    }

    vec3 noise = vec3(random(tuv));
    float player_health_percent = player_health / 100.0;
    float noise_percent = (0.1 * (1.0 - player_health_percent));
    if (disable_noise == 0 && time > flash_start_time && time <= flash_start_time + flash_duration) {
        noise_percent = 0.3;
    }
    if (time > fade_start_time) {
        float fade_percent = (time - fade_start_time) / (fade_end_time - fade_start_time);

        if (disable_noise == 0) {
            noise_percent = 0.3 + (0.75 * fade_percent);
        } else {
            noise_percent = 0.3 + (0.3 * fade_percent);
        }
    }
    if (disable_noise == 1) {
        noise = vec3(1.0);
    }

    vec3 sampled = vec3(texture(screen_texture, tuv));
    vec3 color = mix(sampled, noise, noise_percent);
    frag_color = vec4(color, 1.0);

    if (tuv.x >= 1.0) frag_color = vec4(0.0);
    if (tuv.y >= 1.0) frag_color = vec4(0.0);
    if (tuv.x <= 0.0) frag_color = vec4(0.0);
    if (tuv.y <= 0.0) frag_color = vec4(0.0);
}
