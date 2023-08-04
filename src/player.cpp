#include "player.hpp"

#include "globals.hpp"
#include "input.hpp"
#include "resource.hpp"
#include "shader.hpp"
#include "raycast.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>

const float ACCELERATION = 0.01f;
const float MAX_VELOCITY = 2.0f;
const glm::vec3 ROTATION_SPEED = glm::vec3(0.0075f, 0.0075f, 0.0075f);

glm::vec3 crosshair_color = glm::vec3(1.0f, 1.0f, 1.0f);
glm::ivec2 crosshair_extents = glm::ivec2(1, 3);
glm::ivec2 crosshair_sideways_extents = glm::ivec2(crosshair_extents.y, crosshair_extents.x);

void Player::init() {
    position = glm::vec3(0.0f, 1.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    basis = glm::mat4(1.0f);
    direction = glm::vec3(basis[2]);
    flashlight_direction = -glm::vec3(basis[2]);
    flashlight_on = false;

    animation.animation_info[ANIMATION_PISTOL_IDLE] = {
        .texture_array = resource_player_pistol,
        .start_frame = 1,
        .end_frame = 1,
        .frame_time = 0.0f,
    };
    animation.animation_info[ANIMATION_PISTOL_FIRE] = {
        .texture_array = resource_player_pistol,
        .start_frame = 2,
        .end_frame = 4,
        .frame_time = 1.0f,
    };
}

void Player::update(float delta) {
    // rotation input
    glm::vec3 rotation_input = glm::vec3(0.0f, -input.mouse_raw_yrel, -input.mouse_raw_xrel);
    if (input.is_action_pressed[INPUT_YAW_ROLL]) {
        rotation_input.x = rotation_input.z;
        rotation_input.z = 0;
    }

    // rotate!
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.x * ROTATION_SPEED.x * delta, glm::vec3(basis[2])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.y * ROTATION_SPEED.y * delta, glm::vec3(basis[0])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.z * ROTATION_SPEED.z * delta, glm::vec3(basis[1])) * basis;

    // lerp flashlight
    if (input.is_action_just_pressed[INPUT_FLASHLIGHT]) {
        flashlight_on = !flashlight_on;
    }
    direction = direction + ((glm::vec3(basis[2]) - direction) * 2.0f * delta);
    flashlight_direction = flashlight_direction + ((-glm::vec3(basis[2]) - flashlight_direction) * delta);

    // calculate acceleration
    glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    if (input.is_action_pressed[INPUT_FORWARD]) {
        acceleration -= glm::vec3(basis[2]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_BACKWARD]) {
        acceleration += glm::vec3(basis[2]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_RIGHT]) {
        acceleration += glm::vec3(basis[0]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_LEFT]) {
        acceleration -= glm::vec3(basis[0]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_UP]) {
        acceleration += glm::vec3(basis[1]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_DOWN]) {
        acceleration -= glm::vec3(basis[1]) * ACCELERATION;
    }

    // update velocity
    velocity += acceleration * delta;
    if (glm::length(velocity) > MAX_VELOCITY) {
        velocity = glm::normalize(velocity) * MAX_VELOCITY;
    }

    if (input.is_action_just_pressed[INPUT_LCLICK]) {
        if (animation.animation == ANIMATION_PISTOL_IDLE || (animation.animation == ANIMATION_PISTOL_FIRE && animation.frame >= 2)) {
            animation.set_animation(ANIMATION_PISTOL_FIRE);

            RaycastResult result = raycast_cast(position, direction, 100.0f, 100);
            if (result.hit) {
                printf("hit! %f %f %f\n", result.point.x, result.point.y, result.point.z);
            } else {
                printf("no hit!\n");
            }
        }
    }

    animation.update(delta);
    if (animation.animation == ANIMATION_PISTOL_FIRE && animation.is_finished) {
        animation.set_animation(0);
    }
}

void Player::render() {
    // Render player gun
    glUseProgram(billboard_shader);
    glm::mat4 unit_mat4 = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flashlight_on"), flashlight_on);
    glUniform3fv(glGetUniformLocation(billboard_shader, "view_pos"), 1, glm::value_ptr(position));
    glm::vec3 normal = glm::normalize(glm::vec3(basis[2]));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.position"), 1, glm::value_ptr(position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.direction"), 1, glm::value_ptr(flashlight_direction));
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation.frame);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, animation.animation_info[animation.animation].texture_array);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Render crosshair
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "extents"), 1, glm::value_ptr(crosshair_extents));
    glUniform3fv(glGetUniformLocation(ui_shader, "u_color"), 1, glm::value_ptr(crosshair_color));
    glBlendFunc(GL_ONE, GL_ZERO);
    glBindVertexArray(quad_vao);

    // top part
    glm::ivec2 crosshair_position = glm::ivec2(0, 8);
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // bottom part
    crosshair_position.y *= -1;
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // left part
    glUniform2iv(glGetUniformLocation(ui_shader, "extents"), 1, glm::value_ptr(crosshair_sideways_extents));
    crosshair_position = glm::ivec2(crosshair_position.y, crosshair_position.x);
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // right part
    crosshair_position.x *= -1;
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}
