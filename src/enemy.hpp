#pragma once

#include "animation.hpp"

#include <glm/glm.hpp>

struct Enemy {
    glm::vec3 position;
    glm::vec3 direction;

    Animation animation;

    Enemy();
    void update(float delta);
    void render(glm::vec3 player_position);
};
