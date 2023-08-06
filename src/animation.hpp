#pragma once

#include <map>

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
    std::map<unsigned int, AnimationInfo> animation_info;

    Animation();
    void add_animation(unsigned int name, AnimationInfo info);
    void set_animation(unsigned int name);
    void update(float delta);
};
