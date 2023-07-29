#include "edit.hpp"

#include "globals.hpp"
#include "input.hpp"
#include "level.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <algorithm>
#include <string>

enum Mode {
    MODE_SECTOR,
    MODE_VERTEX,
    MODE_WALL
};

enum SelectionType {
    SELECTING_NOTHING,
    SELECTING_SECTOR,
    SELECTING_VERTEX
};

SDL_Window* edit_window;
SDL_Renderer* renderer;

TTF_Font* font;

const unsigned int UI_WIDTH = 128;
SDL_Rect ui_rect = {
    .x = SCREEN_WIDTH - UI_WIDTH,
    .y = 0,
    .w = UI_WIDTH,
    .h = SCREEN_HEIGHT
};

unsigned int scale = 4;
unsigned int viewport_width = (SCREEN_WIDTH - UI_WIDTH) / scale;
unsigned int viewport_height = SCREEN_HEIGHT / scale;

glm::ivec2 camera_offset = glm::ivec2(viewport_width / 2, viewport_height / 2);

Mode mode = MODE_SECTOR;
SelectionType selection_type = SELECTING_SECTOR;
std::vector<unsigned int> selected_sectors;
std::vector<unsigned int> selected_vertices;
bool dragging = false;
glm::ivec2 drag_origin;
bool changing_floor_or_ceiling = false;
unsigned int text_y_offset;
std::vector<SDL_Rect> ui_hover_box;
int ui_hover_index = -1;

bool is_mouse_in_rect(SDL_Rect& r) {
    return !(input.mouse_raw_x < r.x || input.mouse_raw_x > r.x + r.w || input.mouse_raw_y < r.y || input.mouse_raw_y > r.y + r.h);
}

void refresh_ui_boxes() {
    ui_hover_box.clear();
    for (unsigned int i = 0; i < selected_sectors.size(); i++) {
        int lines_of_text = 3;
        ui_hover_box.push_back({
            .x = ui_rect.x + 2,
            .y = ui_rect.y + 19 + ((int)i * 12 * lines_of_text),
            .w = ui_rect.w - 4,
            .h = 12 * lines_of_text
        });
    }
}

void edit_render_ui_text(std::string text);
void edit_render_text(std::string text, int x, int y);

bool edit_init() {
    glm::ivec2 window_position;
    SDL_Rect display_bounds;
    SDL_GetDisplayBounds(0, &display_bounds);
    window_position.x = (display_bounds.w / 2) - (SCREEN_WIDTH / 4) - SCREEN_WIDTH;
    window_position.y = (display_bounds.h / 2) - (SCREEN_HEIGHT / 2);

    if(TTF_Init() == -1) {
        printf("Unable to initialize SDL_ttf! SDL Error: %s\n", TTF_GetError());
        return false;
    }
    font = TTF_OpenFont("./hack.ttf", 12);
    if (font == NULL) {
        printf("Unable to open font! SDL Error: %s\n", TTF_GetError());
        return false;
    }

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

void edit_update() {
    ui_hover_index = -1;

    if (input.mouse_raw_x < ui_rect.x) {
        // get mouse coordinate position
        glm::ivec2 mouse_snapped_position = glm::ivec2(input.mouse_raw_x / 4, input.mouse_raw_y / 4) + camera_offset;
        if (input.is_action_pressed[INPUT_CTRL]) {
            mouse_snapped_position = (mouse_snapped_position / 8) * 8;
        }

        // camera panning
        if (input.is_action_pressed[INPUT_RCLICK]) {
            camera_offset += glm::ivec2((int)(input.mouse_raw_xrel / 4.0f), (int)(input.mouse_raw_yrel / 4.0f));
        }

        // stop dragging object
        if (input.is_action_just_released[INPUT_LCLICK] && dragging) {
            dragging = false;
            drag_origin = mouse_snapped_position;
            level_init_sectors();
        // select object
        } else if (input.is_action_just_released[INPUT_LCLICK] && !dragging) {
            if (!input.is_action_pressed[INPUT_CTRL]) {
                selected_sectors.clear();
            }
            for (unsigned int i = 0; i < sectors.size(); i++) {
                for (unsigned int j = 0; j < sectors[i].vertices.size(); j++) {
                    SDL_Rect vertex_screen_rect = {
                        .x = (4 * ((int)(sectors[i].vertices[j].x * 8.0f) + camera_offset.x) - 2),
                        .y = (4 * ((int)(sectors[i].vertices[j].y * 8.0f) + camera_offset.y) - 2),
                        .w = 8,
                        .h = 8
                    };

                    if (is_mouse_in_rect(vertex_screen_rect)) {
                        selected_sectors.push_back(i);
                        break;
                    }
                }
            }

            refresh_ui_boxes();
        }

        // begin dragging object
        if (input.is_action_just_pressed[INPUT_LCLICK]) {
            drag_origin = mouse_snapped_position;
        }
        // handle dragging object
        if (input.is_action_pressed[INPUT_LCLICK] && (mouse_snapped_position.x != drag_origin.x || mouse_snapped_position.y != drag_origin.y)) {
            dragging = true;

            glm::vec2 drag_movement = glm::vec2(mouse_snapped_position - drag_origin) / 8.0f;
            drag_origin = mouse_snapped_position;

            if (mode == MODE_SECTOR) {
                for (unsigned int sector_index : selected_sectors) {
                    for (unsigned int j = 0; j < sectors[sector_index].vertices.size(); j++) {
                        sectors[sector_index].vertices[j] += drag_movement;
                    }
                }
            }
        }
    // mouse is inside ui rect
    } else {
        for (unsigned int i = 0; i < ui_hover_box.size(); i++) {
            if (is_mouse_in_rect(ui_hover_box[i])) {
                ui_hover_index = i;
                break;
            }
        }

        if (input.is_action_just_pressed[INPUT_LCLICK] && ui_hover_index != -1) {
            if (mode == MODE_SECTOR) {
                selected_sectors.erase(selected_sectors.begin() + ui_hover_index);
                refresh_ui_boxes();
            }
        }

        if (input.is_action_pressed[INPUT_UP] && ui_hover_index != -1 && input.mouse_raw_yrel != 0) {
            sectors[selected_sectors[ui_hover_index]].ceiling_y -= input.mouse_raw_yrel;
            if (sectors[selected_sectors[ui_hover_index]].ceiling_y <= sectors[selected_sectors[ui_hover_index]].floor_y) {
                sectors[selected_sectors[ui_hover_index]].ceiling_y = sectors[selected_sectors[ui_hover_index]].floor_y + 1;
            }
            changing_floor_or_ceiling = true;
        }

        if (input.is_action_pressed[INPUT_DOWN] && ui_hover_index != -1 && input.mouse_raw_yrel != 0) {
            sectors[selected_sectors[ui_hover_index]].floor_y -= input.mouse_raw_yrel;
            if (sectors[selected_sectors[ui_hover_index]].floor_y >= sectors[selected_sectors[ui_hover_index]].ceiling_y) {
                sectors[selected_sectors[ui_hover_index]].floor_y = sectors[selected_sectors[ui_hover_index]].ceiling_y - 1;
            }
            changing_floor_or_ceiling = true;
        }

        if ((input.is_action_just_released[INPUT_UP] || input.is_action_just_released[INPUT_DOWN]) && changing_floor_or_ceiling) {
            changing_floor_or_ceiling = false;
            level_init_sectors();
        }
    }
}

void edit_render() {
    SDL_SetRenderDrawColor(renderer, 5, 61, 125, 255);
    SDL_RenderClear(renderer);

    SDL_RenderSetScale(renderer, scale, scale);

    // draw gridlines
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_SetRenderDrawColor(renderer, 31, 42, 42, 255);
    unsigned int num_grids_x = (viewport_width / 8) + 1;
    unsigned int grid_start_x = (camera_offset.x % 8);
    for (unsigned int x = 0; x < num_grids_x; x++) {
        SDL_RenderDrawLine(renderer, grid_start_x + (x * 8), 0, grid_start_x + (x * 8), viewport_height);
    }
    unsigned int num_grids_y = (viewport_height / 8) + 1;
    unsigned int grid_start_y = (camera_offset.y % 8);
    for (unsigned int y = 0; y < num_grids_y; y++) {
        SDL_RenderDrawLine(renderer, 0, grid_start_y + (y * 8), viewport_width, grid_start_y + (y * 8));
    }

    // draw walls
    for (unsigned int i = 0; i < sectors.size(); i++) {
        for (unsigned int j = 0; j < sectors[i].vertices.size(); j++) {
            bool i_is_selected = std::find(selected_sectors.begin(), selected_sectors.end(), i) != selected_sectors.end();
            if (i_is_selected) {
                continue;
            }

            SDL_Rect v = {
                .x = (int)(sectors[i].vertices[j].x * 8.0f) + camera_offset.x,
                .y = (int)(sectors[i].vertices[j].y * 8.0f) + camera_offset.y,
                .w = 1,
                .h = 1
            };

            unsigned int other_j = (j + 1) % sectors[i].vertices.size();
            SDL_Rect v2 = {
                .x = (int)(sectors[i].vertices[other_j].x * 8.0f) + camera_offset.x,
                .y = (int)(sectors[i].vertices[other_j].y * 8.0f) + camera_offset.y,
                .w = 1,
                .h = 1
            };

            if (sectors[i].walls[j].exists) {
                SDL_SetRenderDrawColor(renderer, 120, 133, 124, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 62, 84, 84, 255);
            }
            SDL_RenderDrawLine(renderer, v.x, v.y, v2.x, v2.y);
        }
    }

    // draw vertices
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (unsigned int i = 0; i < sectors.size(); i++) {
        for (unsigned int j = 0; j < sectors[i].vertices.size(); j++) {
            bool i_is_selected = std::find(selected_sectors.begin(), selected_sectors.end(), i) != selected_sectors.end();
            if (i_is_selected) {
                continue;
            }

            SDL_Rect v = {
                .x = (int)(sectors[i].vertices[j].x * 8.0f) + camera_offset.x,
                .y = (int)(sectors[i].vertices[j].y * 8.0f) + camera_offset.y,
                .w = 1,
                .h = 1
            };

            SDL_RenderDrawRect(renderer, &v);
        }
    }

    if (selection_type == SELECTING_SECTOR || selection_type == SELECTING_VERTEX) {
        for (unsigned int selected_sector : selected_sectors) {
            // selected sector walls
            for (unsigned int j = 0; j < sectors[selected_sector].vertices.size(); j++) {
                SDL_Rect v = {
                    .x = (int)(sectors[selected_sector].vertices[j].x * 8.0f) + camera_offset.x,
                    .y = (int)(sectors[selected_sector].vertices[j].y * 8.0f) + camera_offset.y,
                    .w = 1,
                    .h = 1
                };
                unsigned int other_j = (j + 1) % sectors[selected_sector].vertices.size();
                SDL_Rect v2 = {
                    .x = (int)(sectors[selected_sector].vertices[other_j].x * 8.0f) + camera_offset.x,
                    .y = (int)(sectors[selected_sector].vertices[other_j].y * 8.0f) + camera_offset.y,
                    .w = 1,
                    .h = 1
                };

                bool j_is_selected = std::find(selected_vertices.begin(), selected_vertices.end(), j) != selected_vertices.end();
                bool other_j_is_selected = std::find(selected_vertices.begin(), selected_vertices.end(), other_j) != selected_vertices.end();

                bool wall_is_yellow = selection_type == SELECTING_SECTOR || (selection_type == SELECTING_VERTEX && j_is_selected && other_j_is_selected);
                if (wall_is_yellow && sectors[selected_sector].walls[j].exists) {
                    SDL_SetRenderDrawColor(renderer, 130, 130, 0, 255);
                } else if (wall_is_yellow && !sectors[selected_sector].walls[j].exists) {
                    SDL_SetRenderDrawColor(renderer, 80, 80, 0, 255);
                } else if (sectors[selected_sector].walls[j].exists) {
                    SDL_SetRenderDrawColor(renderer, 120, 133, 124, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 62, 84, 84, 255);
                }
                SDL_RenderDrawLine(renderer, v.x, v.y, v2.x, v2.y);
            }

            //selected sector vertices
            for (unsigned int j = 0; j < sectors[selected_sector].vertices.size(); j++) {
                SDL_Rect v = {
                    .x = (int)(sectors[selected_sector].vertices[j].x * 8.0f) + camera_offset.x,
                    .y = (int)(sectors[selected_sector].vertices[j].y * 8.0f) + camera_offset.y,
                    .w = 1,
                    .h = 1
                };
                bool j_is_selected = std::find(selected_vertices.begin(), selected_vertices.end(), j) != selected_vertices.end();

                if (selection_type == SELECTING_SECTOR || (selection_type == SELECTING_VERTEX && j_is_selected)) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                }
                SDL_RenderDrawRect(renderer, &v);
            }
        }
    }

    // Render UI
    SDL_RenderSetScale(renderer, 1, 1);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &ui_rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if (ui_hover_index != -1) {
        SDL_RenderDrawRect(renderer, &ui_hover_box[ui_hover_index]);
    }

    text_y_offset = 5;
    if (mode == MODE_SECTOR) {
        edit_render_ui_text("Sector Mode");
        for (unsigned int selected_sector : selected_sectors) {
            edit_render_ui_text("Sector " + std::to_string(selected_sector));
            edit_render_ui_text("ceil: " + std::to_string(sectors[selected_sector].ceiling_y));
            edit_render_ui_text("floor: " + std::to_string(sectors[selected_sector].floor_y));
        }
    }

    SDL_RenderPresent(renderer);
}

void edit_render_ui_text(std::string text) {
    edit_render_text(text, ui_rect.x + 5, ui_rect.y + text_y_offset);
    text_y_offset += 12;
}

void edit_render_text(std::string text, int x, int y) {
    SDL_Color color = {
        .r = 255,
        .g = 255,
        .b = 255,
        .a = 255
    };

    SDL_Surface* text_surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if(text_surface == NULL){

        printf("Unable to render text to surface! SDL Error: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);

    if(text_texture == NULL){

        printf("Unable to reate texture! SDL Error: %s\n", SDL_GetError());
        return;
    }

    SDL_Rect source_rect = (SDL_Rect){ .x = 0, .y = 0, .w = text_surface->w, .h = text_surface->h };
    SDL_Rect dest_rect = (SDL_Rect){ .x = x, .y = y, .w = text_surface->w, .h = text_surface->h };
    SDL_RenderCopy(renderer, text_texture, &source_rect, &dest_rect);

    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_texture);
}
