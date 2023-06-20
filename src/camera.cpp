#include "camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position) {
    this->position = position;
    world_up = glm::vec3(0.0f, 1.0f, 0.0f);
    this->yaw = -90.0f;
    this->pitch = 0.0f;

    front = glm::vec3(0.0f, 0.0f, -1.0f);

    update_camera_vectors();
}

glm::mat4 Camera::get_view_matrix() {
    return glm::lookAt(position, position + front, up);
}

void Camera::handle_mouse_input(float x_offset, float y_offset, GLboolean constrain_pitch) {
    x_offset *= SENSITIVITY;
    y_offset *= SENSITIVITY;

    yaw += x_offset;
    pitch += y_offset;

    if (constrain_pitch) {
        if (pitch > 89.0f) {
            pitch = 89.0f;
        }
        if (pitch < -89.0f) {
            pitch = -89.0f;
        }
    }

    update_camera_vectors();
}

void Camera::update(float delta) {
    float velocity = SPEED * delta;
    if (input_direction.y == -1) {
        position += front * velocity;
    } else if (input_direction.y == 1) {
        position -= front * velocity;
    } else if (input_direction.x == -1) {
        position -= right * velocity;
    } else if (input_direction.x == 1) {
        position += right * velocity;
    }
}

void Camera::update_camera_vectors() {
    front = glm::normalize(glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))));
    right = glm::normalize(glm::cross(front, world_up));
    up = glm::normalize(glm::cross(right, front));
}
