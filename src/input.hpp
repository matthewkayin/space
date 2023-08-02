#pragma once

#include <SDL2/SDL.h>

enum Input {
    INPUT_LCLICK,
    INPUT_RCLICK,
    INPUT_CTRL,
    INPUT_DELETE,
    INPUT_T,
    INPUT_G,
    INPUT_FORWARD,
    INPUT_BACKWARD,
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_YAW_ROLL,
    INPUT_FLASHLIGHT,
    INPUT_COUNT
};

struct InputState {
    bool is_action_pressed[INPUT_COUNT];
    bool is_action_just_pressed[INPUT_COUNT];
    bool is_action_just_released[INPUT_COUNT];
    float mouse_raw_xrel;
    float mouse_raw_yrel;
    float mouse_raw_x;
    float mouse_raw_y;
    float mouse_x;
    float mouse_y;
};

extern InputState input;

void input_set_mapping();
void input_prime_state();
void input_handle_event(SDL_Event e);
