#include "player.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

#include "input.hpp"

const float MAX_VELOCITY = 2.0f;
const float ACCELERATION = 0.01f;
const glm::vec3 MAX_ROTATION_SPEED = glm::vec3(0.2f, 0.1f, 0.2f);
const glm::vec3 ROTATION_ACCEL = glm::vec3(0.05f, 0.05f, 0.05f);
const float ROTATION_DECEL = 0.025f;

Player::Player() {
    position = glm::vec3(0.0f, 1.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    basis = glm::mat4(1.0f);
    rotation_speed = glm::vec3(0.0f, 0.0f, 0.0f);
    flashlight_direction = -glm::vec3(basis[2]);
    flashlight_on = false;
}

void Player::update(float delta) {
    // rotation deceleration
    for (unsigned int i = 0; i < 3; i++) {
        if (std::fabs(rotation_speed[i]) > ROTATION_DECEL * delta) {
            if (rotation_speed[i] > 0.0f) {
                rotation_speed[i] -= ROTATION_DECEL * delta;
            } else {
                rotation_speed[i] += ROTATION_DECEL * delta;
            }
        } else {
            rotation_speed[i] = 0.0f;
        }
    }

    // rotation input
    glm::vec3 rotation_input = glm::vec3(0.0f, -input.mouse_y, -input.mouse_x);
    if (input.is_action_pressed[INPUT_YAW_ROLL]) {
        rotation_input.x = rotation_input.z;
        rotation_input.z = 0;
    }

    // update rotation speed
    for (unsigned int i = 0; i < 3; i++) {
        rotation_speed[i] += rotation_input[i] * ROTATION_ACCEL[i] * delta;
        rotation_speed[i] = std::fmin(std::fmax(rotation_speed[i], -MAX_ROTATION_SPEED[i]), MAX_ROTATION_SPEED[i]);
    }

    // rotate!
    basis = glm::rotate(glm::mat4(1.0f), rotation_speed.x * delta, glm::vec3(basis[2])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_speed.y * delta, glm::vec3(basis[0])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_speed.z * delta, glm::vec3(basis[1])) * basis;

    // lerp flashlight
    if (input.is_action_just_pressed[INPUT_FLASHLIGHT]) {
        flashlight_on = !flashlight_on;
    }
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
}
