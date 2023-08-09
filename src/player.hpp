#pragma once

#include "animation.hpp"
#include "raycast.hpp"

#include <glm/glm.hpp>

enum ScreenAnimation {
    SCREEN_ANIMATION_NONE,
    SCREEN_ANIMATION_FLASH,
    SCREEN_ANIMATION_FADE
};

struct Player {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::mat4 basis;
    glm::vec3 direction;

    bool flashlight_on;
    glm::vec3 flashlight_direction;

    Animation animation;
    ScreenAnimation screen_animation;

    unsigned int reserve_ammo;
    unsigned int clip_ammo;
    unsigned int clip_max;

    float recoil;
    float recoil_cooldown;

    unsigned int health;
    unsigned int max_health;
    bool is_dead;

    RaycastResult raycast_result;

    Player() {};
    void init();
    void update(float delta);
    void take_damage(unsigned int amount);
    void render();
};
