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
    input_map.insert({ SDLK_LCTRL, INPUT_CTRL });
    input_map.insert({ SDLK_DELETE, INPUT_DELETE });
    input_map.insert({ SDLK_t, INPUT_T });
    input_map.insert({ SDLK_g, INPUT_G });
}

void input_prime_state() {
    for (unsigned int i = 0; i < INPUT_COUNT; i++) {
        input.is_action_just_pressed[i] = false;
        input.is_action_just_released[i] = false;
    }
    input.mouse_raw_xrel = 0.0f;
    input.mouse_raw_yrel = 0.0f;
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
        input.is_action_just_released[mapping->second] = true;
    } else if (e.type == SDL_MOUSEMOTION) {
        input.mouse_raw_xrel = e.motion.xrel;
        input.mouse_raw_yrel = e.motion.yrel;
        input.mouse_raw_x = e.motion.x;
        input.mouse_raw_y = e.motion.y;

        if (abs(e.motion.xrel) > MOUSE_DEADZONE) {
            input.mouse_x = std::max(std::min((float)e.motion.xrel / MOUSE_MAX, 1.0f), -1.0f);
        }
        if (abs(e.motion.yrel) > MOUSE_DEADZONE) {
            input.mouse_y = std::max(std::min((float)e.motion.yrel / MOUSE_MAX, 1.0f), -1.0f);
        }
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            input.is_action_just_pressed[INPUT_LCLICK] = true;
            input.is_action_pressed[INPUT_LCLICK] = true;
        } else if (e.button.button == SDL_BUTTON_RIGHT) {
            input.is_action_just_pressed[INPUT_RCLICK] = true;
            input.is_action_pressed[INPUT_RCLICK] = true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            input.is_action_pressed[INPUT_LCLICK] = false;
            input.is_action_just_released[INPUT_LCLICK] = true;
        } else if (e.button.button == SDL_BUTTON_RIGHT) {
            input.is_action_pressed[INPUT_RCLICK] = false;
            input.is_action_just_released[INPUT_RCLICK] = true;
        }
    }
}
