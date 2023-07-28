#include "edit.hpp"

#include "globals.hpp"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <cstdio>

SDL_Window* edit_window;
SDL_Renderer* renderer;

bool edit_init() {
    glm::ivec2 window_position;
    SDL_Rect display_bounds;
    SDL_GetDisplayBounds(0, &display_bounds);
    window_position.x = (display_bounds.w / 2) - (SCREEN_WIDTH / 4) - SCREEN_WIDTH;
    window_position.y = (display_bounds.h / 2) - (SCREEN_HEIGHT / 2);

    edit_window = SDL_CreateWindow("zerog", window_position.x, window_position.y, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (edit_window == NULL) {
        printf("Error creating edit window: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(edit_window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Error creating edit window renderer: %s\n", SDL_GetError());
        return -1;
    }

    return true;
}

void edit_quit() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(edit_window);
}

void edit_render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}
