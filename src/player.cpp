#include "player.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

#include "input.hpp"

Player::Player() {
    position = glm::vec3(0.0f, 0.5f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    basis = glm::mat4(1.0f);
}

void Player::update(float delta) {
    // rotation
    glm::vec3 rotation_input = glm::vec3(0.0f, -input.mouse_yrel, -input.mouse_xrel) * 0.01f;
    if (input.is_action_pressed[INPUT_ROTATE_UP]) {
        rotation_input.y = 0.1f;
    } else if (input.is_action_pressed[INPUT_ROTATE_DOWN]) {
        rotation_input.y = -0.1f;
    }
    if (input.is_action_pressed[INPUT_ROTATE_RIGHT]) {
        rotation_input.z = -0.1f;
    } else if (input.is_action_pressed[INPUT_ROTATE_LEFT]) {
        rotation_input.z = 0.1f;
    }
    if (input.is_action_pressed[INPUT_YAW_ROLL]) {
        rotation_input.x = rotation_input.z;
        rotation_input.z = 0;
    }
    rotation_input *= delta;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.x, glm::vec3(basis[2])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.y, glm::vec3(basis[0])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.z, glm::vec3(basis[1])) * basis;
    /*for (unsigned int i = 0; i < 4;i++) {
        printf("%f %f %f %f\n", basis[i][0], basis[i][1], basis[i][2], basis[i][3]);
    }
    printf("\n");*/

    if (input.is_action_pressed[INPUT_FORWARD]) {
        velocity -= glm::vec3(basis[2]) * 0.01f;
    }
    if (input.is_action_pressed[INPUT_BACKWARD]) {
        velocity += glm::vec3(basis[2]) * 0.01f;
    }
    if (input.is_action_pressed[INPUT_RIGHT]) {
        velocity += glm::vec3(basis[0]) * 0.01f;
    }
    if (input.is_action_pressed[INPUT_LEFT]) {
        velocity -= glm::vec3(basis[0]) * 0.01f;
    }
    if (input.is_action_pressed[INPUT_UP]) {
        velocity += glm::vec3(basis[1]) * 0.01f;
    }
    if (input.is_action_pressed[INPUT_DOWN]) {
        velocity -= glm::vec3(basis[1]) * 0.01f;
    }
    position += velocity * delta;
}
