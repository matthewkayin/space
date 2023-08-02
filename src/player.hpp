#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

struct Player {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::mat4 basis;
    glm::vec3 rotation_speed;
    glm::vec3 flashlight_direction;
    bool flashlight_on;

    Player();
    void update(float delta);
};
