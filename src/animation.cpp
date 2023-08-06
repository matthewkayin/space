#include "animation.hpp"

Animation::Animation() {
    frame = 0;
    animation = 0;
    timer = 0.0f;
    is_finished = false;
}

void Animation::add_animation(unsigned int name, AnimationInfo info) {
    animation_info.insert(std::pair<unsigned int, AnimationInfo>(name, info));
}

void Animation::set_animation(unsigned int name) {
    animation = name;
    frame = animation_info[animation].start_frame;
    timer = 0.0f;
}

void Animation::update(float delta) {
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
