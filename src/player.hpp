#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

enum AnimationPistol {
    ANIMATION_PISTOL_IDLE,
    ANIMATION_PISTOL_FIRE,
    ANIMATION_COUNT
};

struct AnimationInfo {
    unsigned int texture_array;
    unsigned int start_frame;
    unsigned int end_frame;
    float frame_time;
};

struct Animation {
    unsigned int frame;
    unsigned int animation;
    float timer;
    bool is_finished;
    AnimationInfo animation_info[ANIMATION_COUNT];

    Animation() {
        frame = 0;
        animation = 0;
        timer = 0.0f;
        is_finished = false;
    }

    void set_animation(unsigned int animation) {
        this->animation = animation;
        frame = 0;
        timer = 0.0f;
    }

    void update(float delta) {
        is_finished = false;
        if (animation_info[animation].start_frame == animation_info[animation].end_frame) {
            return;
        }

        timer += delta;
        while (timer >= animation_info[animation].frame_time) {
            timer -= animation_info[animation].frame_time;
            frame++;
            if (frame > animation_info[animation].end_frame) {
                frame = animation_info[animation].start_frame;
                is_finished = true;
            }
        }
    }
};


struct Player {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::mat4 basis;
    glm::vec3 direction;
    glm::vec3 flashlight_direction;
    bool flashlight_on;

    Animation animation;

    std::vector<RaycastResult> bullet_hits;

    void init();
    void update(float delta);
    void render();
};
