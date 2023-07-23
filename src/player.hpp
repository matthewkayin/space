#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

struct Player {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::mat4 basis;

    Player();
    void update(float delta);
};
