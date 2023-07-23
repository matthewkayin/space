#pragma once

#include <SDL2/SDL.h>

enum Input {
    INPUT_FORWARD,
    INPUT_BACKWARD,
    INPUT_RIGHT,
    INPUT_LEFT,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_YAW_ROLL,
    INPUT_ROTATE_UP,
    INPUT_ROTATE_DOWN,
    INPUT_ROTATE_RIGHT,
    INPUT_ROTATE_LEFT,
    INPUT_COUNT
};

struct InputState {
    bool is_action_pressed[INPUT_COUNT];
    bool is_action_just_pressed[INPUT_COUNT];
    float mouse_xrel;
    float mouse_yrel;
};

extern InputState input;

void input_set_mapping();
void input_prime_state();
void input_handle_event(SDL_Event e);
