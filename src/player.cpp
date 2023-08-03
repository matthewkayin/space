#include "player.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

#include "input.hpp"

const float ACCELERATION = 0.01f;
const float MAX_VELOCITY = 2.0f;
const glm::vec3 ROTATION_SPEED = glm::vec3(0.0075f, 0.0075f, 0.0075f);

Player::Player() {
    position = glm::vec3(0.0f, 1.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    basis = glm::mat4(1.0f);
    direction = glm::vec3(basis[2]);
    flashlight_direction = -glm::vec3(basis[2]);
    flashlight_on = false;
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
}
