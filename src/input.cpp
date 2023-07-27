#include "input.hpp"

#include <map>
#include <cstdio>

const int MOUSE_DEADZONE = 3;
const int MOUSE_MAX = 10;

InputState input;

std::map<unsigned int, Input> input_map;

void input_set_mapping() {
    input_map.insert({ SDLK_w, INPUT_FORWARD });
    input_map.insert({ SDLK_s, INPUT_BACKWARD });
    input_map.insert({ SDLK_d, INPUT_RIGHT });
    input_map.insert({ SDLK_a, INPUT_LEFT });
    input_map.insert({ SDLK_e, INPUT_UP });
    input_map.insert({ SDLK_q, INPUT_DOWN });
    input_map.insert({ SDLK_LSHIFT, INPUT_YAW_ROLL });
    input_map.insert({ SDLK_UP, INPUT_ROTATE_UP });
    input_map.insert({ SDLK_DOWN, INPUT_ROTATE_DOWN });
    input_map.insert({ SDLK_RIGHT, INPUT_ROTATE_RIGHT });
    input_map.insert({ SDLK_LEFT, INPUT_ROTATE_LEFT });
}

void input_prime_state() {
    for (unsigned int i = 0; i < INPUT_COUNT; i++) {
        input.is_action_just_pressed[i] = false;
    }
    input.mouse_x = 0.0f;
    input.mouse_y = 0.0f;
}

void input_handle_event(SDL_Event e) {
    if (e.type == SDL_KEYDOWN) {
        std::map<unsigned int, Input>::iterator mapping = input_map.find(e.key.keysym.sym);
        if (mapping == input_map.end()) {
            return;
        }

        input.is_action_pressed[mapping->second] = true;
        input.is_action_just_pressed[mapping->second] = true;
    } else if (e.type == SDL_KEYUP) {
        std::map<unsigned int, Input>::iterator mapping = input_map.find(e.key.keysym.sym);
        if (mapping == input_map.end()) {
            return;
        }

        input.is_action_pressed[mapping->second] = false;
    } else if (e.type == SDL_MOUSEMOTION) {
        if (abs(e.motion.xrel) > MOUSE_DEADZONE) {
            input.mouse_x = std::max(std::min((float)e.motion.xrel / MOUSE_MAX, 1.0f), -1.0f);
        }
        if (abs(e.motion.yrel) > MOUSE_DEADZONE) {
            input.mouse_y = std::max(std::min((float)e.motion.yrel / MOUSE_MAX, 1.0f), -1.0f);
        }
    }
}
