#pragma once

#include "animation.hpp"

#include <glm/glm.hpp>
#include <vector>

struct EnemyBulletHole {
    glm::vec3 position;
    glm::vec3 normal;
    Animation animation;

    EnemyBulletHole(glm::vec3 position, glm::vec3 normal);
    void update(float delta);
    void render();
};

struct Enemy {
    glm::vec3 position;
    glm::vec3 direction;

    glm::vec2 hurtbox_extents;
    unsigned int hurtbox_raycast_plane;

    Animation animation;
    unsigned int animation_offset;
    bool flip_h;

    Enemy();
    void update(float delta);
    void render(glm::vec3 player_position);
};
