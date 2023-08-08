#include "edit_scene.hpp"

#include "resource.hpp"
#include "shader.hpp"
#include "level.hpp"
#include "globals.hpp"
#include "input.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

float camera_yaw = -90.0f;
float camera_pitch = 0.0f;
glm::vec3 camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_position = glm::vec3(0.0f, 0.5f, 0.0f);

bool lighting_enabled = true;

void edit_scene_init() {
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUseProgram(billboard_shader);
    glUniform1i(glGetUniformLocation(billboard_shader, "u_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUniform2iv(glGetUniformLocation(billboard_shader, "screen_size"), 1, glm::value_ptr(screen_size));
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "screen_size"), 1, glm::value_ptr(screen_size));

    camera_position = glm::vec3(0.0f, 1.0f, 0.0f);
}

void edit_scene_update(float delta) {
    camera_yaw += input.mouse_raw_xrel * 0.1f;
    camera_pitch -= input.mouse_raw_yrel * 0.1f;
    camera_pitch = std::min(std::max(camera_pitch, -89.0f), 89.0f);
    camera_direction = glm::normalize(glm::vec3(
                    cos(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch)),
                    sin(glm::radians(camera_pitch)),
                    sin(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch))));

    glm::vec3 camera_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    float camera_speed = 0.2f;
    if (input.is_action_pressed[INPUT_FORWARD]) {
        camera_velocity += camera_direction;
    }
    if (input.is_action_pressed[INPUT_BACKWARD]) {
        camera_velocity += -camera_direction;
    }
    if (input.is_action_pressed[INPUT_UP]) {
        camera_velocity += glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (input.is_action_pressed[INPUT_DOWN]) {
        camera_velocity += glm::vec3(0.0f, -1.0f, 0.0f);
    }
    if (input.is_action_pressed[INPUT_LEFT]) {
        camera_velocity += -glm::normalize(glm::cross(camera_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    if (input.is_action_pressed[INPUT_RIGHT]) {
        camera_velocity += glm::normalize(glm::cross(camera_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    if (glm::length(camera_velocity) > 1.0f) {
        camera_velocity = glm::normalize(camera_velocity);
    }

    camera_position += camera_velocity * camera_speed * delta;

    if (input.is_action_just_pressed[INPUT_FLASHLIGHT]) {
        lighting_enabled = !lighting_enabled;
        glUseProgram(billboard_shader);
        glUniform1ui(glGetUniformLocation(billboard_shader, "lighting_enabled"), lighting_enabled);
        glUseProgram(texture_shader);
        glUniform1ui(glGetUniformLocation(texture_shader, "lighting_enabled"), lighting_enabled);
    }
}

void edit_scene_render() {
    glm::mat4 view;
    view = glm::lookAt(camera_position, camera_position + camera_direction, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 100.0f);

    // prepare billboard shader
    glUseProgram(billboard_shader);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flashlight_on"), false);
    glUniform3fv(glGetUniformLocation(billboard_shader, "view_pos"), 1, glm::value_ptr(camera_position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.position"), 1, glm::value_ptr(camera_position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.direction"), 1, glm::value_ptr(camera_direction));
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), false);

    level_render(view, projection, camera_position, glm::vec3(0.0f), false);

    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_wasp]));
    for (glm::vec3 enemy_spawn_point : enemy_spawn_points) {
        glm::vec3 facing_direction = glm::normalize(glm::vec3(camera_position.x, enemy_spawn_point.y, camera_position.z) - enemy_spawn_point);
        glm::mat4 model = glm::inverse(glm::lookAt(enemy_spawn_point, enemy_spawn_point + facing_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(facing_direction));

        glBindTexture(GL_TEXTURE_2D_ARRAY, resource_wasp);
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}
