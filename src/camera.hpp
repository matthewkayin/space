#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

struct Camera {
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));
    glm::mat4 get_view_matrix();
    void handle_mouse_input(float x_offset, float y_offset, GLboolean constrain_pitch = true);
    void update(float delta);
    void update_camera_vectors();

    glm::vec2 input_direction;
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;
    float yaw;
    float pitch;
    float zoom;
};
