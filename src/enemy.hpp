#pragma once

#include "animation.hpp"
#include "raycast.hpp"

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
    glm::vec3 facing_direction;
    float angle;

    std::vector<EnemyBulletHole> bullet_holes;

    glm::vec2 hurtbox_extents;
    unsigned int hurtbox_raycast_plane;

    Animation animation;
    unsigned int animation_offset;
    bool flip_h;

    int health;
    bool is_dead;

    bool has_seen_player;
    bool hit_player;
    bool has_hit;

    Enemy(unsigned int id);
    void update(glm::vec3 player_position, float delta);
    void take_damage(RaycastResult& result, int amount);
    void render();
};
