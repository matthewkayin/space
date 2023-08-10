#include "edit.hpp"

#include "globals.hpp"
#include "input.hpp"
#include "level.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <algorithm>
#include <string>

enum Mode {
    MODE_SECTOR,
    MODE_VERTEX,
    MODE_NEW_SECTOR,
    MODE_OBJECT,
    MODE_NEW_OBJECT,
    MODE_SAVED_FILE
};

enum ObjectType {
    OBJECT_PLAYER,
    OBJECT_ENEMY
};

struct ObjectSelection {
    ObjectType type;
    unsigned int index;
};

SDL_Window* edit_window;
SDL_Renderer* renderer;

TTF_Font* font;

SDL_Texture* textures[NUM_TEXTURES];

const SDL_Color vertex_color = { .r = 255, .g = 255, .b = 255, .a = 255 };
const SDL_Color wall_color = { .r = 120, .g = 133, .b = 124, .a = 255 };
const SDL_Color hidden_wall_color = { .r = 62, .g = 84, .b = 84, .a = 255 };
const SDL_Color selected_vertex_color = { .r = 255, .g = 255, .b = 0, .a = 255 };
const SDL_Color selected_wall_color = { .r = 130, .g = 130, .b = 0, .a = 255 };
const SDL_Color selected_hidden_wall_color = { .r = 80, .g = 80, .b = 0, .a = 255 };
const SDL_Color vertex_cursor_color = { .r = 255, .g = 255, .b = 255, .a = 128 };

const unsigned int UI_WIDTH = 128;
SDL_Rect ui_rect;
SDL_Rect texture_picker_rect;

unsigned int scale = 4;
unsigned int viewport_width = (WINDOW_WIDTH - UI_WIDTH) / scale;
unsigned int viewport_height = WINDOW_HEIGHT / scale;

glm::ivec2 camera_offset = glm::ivec2(viewport_width / 2, viewport_height / 2);
glm::vec2 precise_camera_offset = glm::vec2(camera_offset);

Mode mode = MODE_SECTOR;
std::vector<unsigned int> selected_sectors;
std::vector<ObjectSelection> object_selections;

bool dragging = false;
int dragging_vertex = -1;
glm::ivec2 drag_origin;
bool changing_floor_or_ceiling = false;

unsigned int text_y_offset;
std::vector<SDL_Rect> ui_hover_box;
int ui_hover_index = -1;

Sector new_sector;
ObjectType new_object_type;

unsigned int current_texture = 0;

bool is_mouse_in_rect(SDL_Rect& r) {
    return !(input.mouse_raw_x < r.x || input.mouse_raw_x > r.x + r.w || input.mouse_raw_y < r.y || input.mouse_raw_y > r.y + r.h);
}

void refresh_ui_boxes() {
    ui_hover_box.clear();
    if (mode == MODE_SECTOR || mode == MODE_VERTEX || mode == MODE_OBJECT) {
        int lines_of_text = 5;
        unsigned int num_boxes = selected_sectors.size();
        if (mode == MODE_VERTEX) {
            lines_of_text = 3;
            num_boxes = sectors[selected_sectors[0]].vertices.size();
        } else if (mode == MODE_OBJECT) {
            lines_of_text = 3;
            num_boxes = object_selections.size();
        }
        for (unsigned int i = 0; i < num_boxes; i++) {
            ui_hover_box.push_back({
                .x = ui_rect.x + 2,
                .y = ui_rect.y + 19 + ((int)i * 12 * lines_of_text),
                .w = ui_rect.w - 4,
                .h = 12 * lines_of_text
            });
        }
    }
}

void edit_render_sector_walls(const Sector& sector, SDL_Color wall_color, SDL_Color non_wall_color);
void edit_render_sector_vertices(const Sector& sector, SDL_Color vertex_color);
void edit_render_ui_text(std::string text);
void edit_render_text(std::string text, int x, int y);

bool edit_init() {
    ui_rect = {
        .x = (int)(WINDOW_WIDTH - UI_WIDTH),
        .y = 0,
        .w = UI_WIDTH,
        .h = (int)WINDOW_HEIGHT
    };
    texture_picker_rect = {
        .x = ui_rect.x,
        .y = (int)(WINDOW_HEIGHT - 64),
        .w = 128,
        .h = 64
    };

    glm::ivec2 window_position;
    SDL_Rect display_bounds;
    SDL_GetDisplayBounds(0, &display_bounds);
    window_position.x = (display_bounds.w / 2) - (WINDOW_WIDTH / 4) - WINDOW_WIDTH;
    window_position.y = (display_bounds.h / 2) - (WINDOW_HEIGHT / 2);

    int img_flags = IMG_INIT_PNG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        printf("Unable to initialize SDL_image! SDL Error: %s\n", IMG_GetError());
        return false;
    }

    if(TTF_Init() == -1) {
        printf("Unable to initialize SDL_ttf! SDL Error: %s\n", TTF_GetError());
        return false;
    }
    font = TTF_OpenFont("./hack.ttf", 12);
    if (font == NULL) {
        printf("Unable to open font! SDL Error: %s\n", TTF_GetError());
        return false;
    }

    edit_window = SDL_CreateWindow("zerog", window_position.x, window_position.y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (edit_window == NULL) {
        printf("Error creating edit window: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(edit_window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Error creating edit window renderer: %s\n", SDL_GetError());
        return -1;
    }

    // load textures
    for (unsigned int i = 0; i < NUM_TEXTURES; i++) {
        SDL_Surface* loaded_surface = IMG_Load(("./res/texture/" + std::to_string(i) + ".png").c_str());
        if(loaded_surface == NULL){

            printf("Unable to load texture image! SDL Error %s\n", IMG_GetError());
            return NULL;
        }

        textures[i] = SDL_CreateTextureFromSurface(renderer, loaded_surface);
        if(textures[i] == NULL){

            printf("Unable to create texture %i! SDL Error: %s\n", i, SDL_GetError());
            return NULL;
        }

        SDL_FreeSurface(loaded_surface);
    }

    return true;
}

void edit_quit() {
    TTF_CloseFont(font);

    for (unsigned int i = 0; i < NUM_TEXTURES; i++) {
        SDL_DestroyTexture(textures[i]);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(edit_window);

    TTF_Quit();
    IMG_Quit();
}

void edit_update() {
    ui_hover_index = -1;

    if (input.mouse_raw_x < ui_rect.x) {
        // get mouse coordinate position
        glm::ivec2 mouse_snapped_position = glm::ivec2(input.mouse_raw_x / 4, input.mouse_raw_y / 4) - camera_offset;
        if (input.is_action_pressed[INPUT_CTRL]) {
            mouse_snapped_position = (mouse_snapped_position / 8) * 8;
        }

        // camera panning
        if (input.is_action_pressed[INPUT_RCLICK]) {
            precise_camera_offset += glm::vec2(input.mouse_raw_xrel / 4.0f, input.mouse_raw_yrel / 4.0f);
            camera_offset = glm::ivec2(precise_camera_offset);
        }

        // stop dragging object
        if (input.is_action_just_released[INPUT_LCLICK] && dragging) {
            dragging = false;
            dragging_vertex = -1;
            drag_origin = mouse_snapped_position;
            level_init_sectors();
        // select object
        } else if (input.is_action_just_released[INPUT_LCLICK] && !dragging) {
            if (mode == MODE_SECTOR) {
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
            } else if (mode == MODE_NEW_SECTOR) {
                new_sector.add_vertex(glm::vec2(mouse_snapped_position) / 8.0f, current_texture, true);
            } else if (mode == MODE_OBJECT) {
                if (!input.is_action_pressed[INPUT_CTRL]) {
                    object_selections.clear();
                }
                SDL_Rect p = {
                    .x = (4 * ((int)(player_spawn_point.x * 8.0f) + camera_offset.x) - 2),
                    .y = (4 * ((int)(player_spawn_point.z * 8.0f) + camera_offset.y) - 2),
                    .w = 8,
                    .h = 8
                };

                if (is_mouse_in_rect(p)) {
                    object_selections.push_back({
                        .type = OBJECT_PLAYER,
                        .index = 0
                    });
                }

                for (unsigned int i = 0; i < enemy_spawns.size(); i++) {
                    p = {
                        .x = (4 * ((int)(enemy_spawns[i].position.x * 8.0f) + camera_offset.x) - 2),
                        .y = (4 * ((int)(enemy_spawns[i].position.z * 8.0f) + camera_offset.y) - 2),
                        .w = 8,
                        .h = 8
                    };

                    if (is_mouse_in_rect(p)) {
                        object_selections.push_back({
                            .type = OBJECT_ENEMY,
                            .index = i
                        });
                    }
                }

                refresh_ui_boxes();
            } else if (mode == MODE_NEW_OBJECT) {
                if (new_object_type == OBJECT_ENEMY) {
                    enemy_spawns.push_back({
                        .position = glm::vec3(mouse_snapped_position.x, 0.0f, mouse_snapped_position.y) / 8.0f,
                        .direction = glm::vec2(0.0f, 1.0f)
                    });
                    mode = MODE_OBJECT;
                    object_selections.push_back({
                        .type = OBJECT_ENEMY,
                        .index = (unsigned int)(enemy_spawns.size() - 1)
                    });
                    refresh_ui_boxes();
                }
            }
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

            if (mode == MODE_VERTEX) {
                for (unsigned int j = 0; j < sectors[selected_sectors[0]].vertices.size(); j++) {
                    SDL_Rect vertex_screen_rect = {
                        .x = (4 * ((int)(sectors[selected_sectors[0]].vertices[j].x * 8.0f) + camera_offset.x) - 2),
                        .y = (4 * ((int)(sectors[selected_sectors[0]].vertices[j].y * 8.0f) + camera_offset.y) - 2),
                        .w = 8,
                        .h = 8
                    };

                    if (is_mouse_in_rect(vertex_screen_rect)) {
                        dragging_vertex = j;
                        break;
                    }
                }
            }

            if (mode == MODE_SECTOR) {
                for (unsigned int sector_index : selected_sectors) {
                    for (unsigned int j = 0; j < sectors[sector_index].vertices.size(); j++) {
                        sectors[sector_index].vertices[j] += drag_movement;
                    }
                }
            } else if (mode == MODE_VERTEX && dragging_vertex != -1) {
                sectors[selected_sectors[0]].vertices[dragging_vertex] += drag_movement;
            } else if (mode == MODE_OBJECT) {
                for (ObjectSelection& object_selection : object_selections) {
                    if (object_selection.type == OBJECT_PLAYER) {
                        player_spawn_point += glm::vec3(drag_movement.x, 0.0f, drag_movement.y);
                    } else if (object_selection.type == OBJECT_ENEMY) {
                        enemy_spawns[object_selection.index].position += glm::vec3(drag_movement.x, 0.0f, drag_movement.y);
                    }
                }
            }
        }

        // exit current mode
        if (input.is_action_just_pressed[INPUT_DOWN] && (mode == MODE_NEW_SECTOR || mode == MODE_VERTEX)) {
            mode = MODE_SECTOR;
            refresh_ui_boxes();
        }

        // begin sector create
        if (mode == MODE_SECTOR && input.is_action_just_pressed[INPUT_LEFT] && !dragging) {
            selected_sectors.clear();
            new_sector = Sector();
            mode = MODE_NEW_SECTOR;
            refresh_ui_boxes();
        // finish sector create, done in an else if so that this don't trigger on the same frame as the mode started
        } else if (mode == MODE_NEW_SECTOR && input.is_action_just_pressed[INPUT_LEFT] && !dragging) {
            new_sector.floor_texture_index = current_texture;
            new_sector.ceiling_texture_index = current_texture;
            sectors.push_back(new_sector);
            selected_sectors.push_back(sectors.size() - 1);
            mode = MODE_SECTOR;
            level_init_sectors();
            refresh_ui_boxes();
        }

        // cancel new sector mode
        if (mode == MODE_NEW_SECTOR && input.is_action_just_pressed[INPUT_DOWN]) {
            mode = MODE_SECTOR;
            refresh_ui_boxes();
        }

        // enter object mode
        if ((mode == MODE_SECTOR || mode == MODE_SAVED_FILE) && !dragging && input.is_action_just_pressed[INPUT_O]) {
            mode = MODE_OBJECT;
            selected_sectors.clear();
            refresh_ui_boxes();
        }

        // enter sector mode
        if ((mode == MODE_OBJECT || mode == MODE_SAVED_FILE) && !dragging && input.is_action_just_pressed[INPUT_BACKWARD]) {
            mode = MODE_SECTOR;
            object_selections.clear();
            refresh_ui_boxes();
        }

        // enter new object mode
        if (mode == MODE_OBJECT && !dragging && input.is_action_just_pressed[INPUT_LEFT]) {
            mode = MODE_NEW_OBJECT;
            new_object_type = OBJECT_ENEMY;
            object_selections.clear();
            refresh_ui_boxes();
        }

        // cancel new object mode
        if (mode == MODE_NEW_OBJECT && input.is_action_just_pressed[INPUT_DOWN]) {
            mode = MODE_OBJECT;
            refresh_ui_boxes();
        }

        // deselect object
        if (mode == MODE_OBJECT && input.is_action_just_pressed[INPUT_RCLICK] && ui_hover_index != -1) {
            object_selections.erase(object_selections.begin() + ui_hover_index);
            refresh_ui_boxes();
        }

        // delete object
        if (mode == MODE_OBJECT && input.is_action_just_pressed[INPUT_DELETE] && ui_hover_index != -1) {
            if (object_selections[ui_hover_index].type == OBJECT_ENEMY) {
                enemy_spawns.erase(enemy_spawns.begin() + object_selections[ui_hover_index].index);
                object_selections.erase(object_selections.begin() + ui_hover_index);
            }
            refresh_ui_boxes();
        }

        // save file
        if (mode != MODE_NEW_OBJECT && mode != MODE_NEW_SECTOR && !dragging && input.is_action_just_pressed[INPUT_FLASHLIGHT]) {
            level_save_file();
            mode = MODE_SAVED_FILE;
            object_selections.clear();
            selected_sectors.clear();
            refresh_ui_boxes();
        }
    // mouse is inside ui rect
    } else {
        for (unsigned int i = 0; i < ui_hover_box.size(); i++) {
            if (is_mouse_in_rect(ui_hover_box[i])) {
                ui_hover_index = i;
                break;
            }
        }

        // deselect sector
        if (mode == MODE_SECTOR && input.is_action_just_pressed[INPUT_RCLICK] && ui_hover_index != -1) {
            selected_sectors.erase(selected_sectors.begin() + ui_hover_index);
            refresh_ui_boxes();
        }

        // go into vertex mode for sector
        if (input.is_action_just_pressed[INPUT_LCLICK] && ui_hover_index != -1) {
            if (mode == MODE_SECTOR) {
                unsigned int selected_sector = selected_sectors[ui_hover_index];
                selected_sectors.clear();
                selected_sectors.push_back(selected_sector);
                mode = MODE_VERTEX;
                refresh_ui_boxes();
            }
        }

        // begin changing ceiling
        if (mode == MODE_SECTOR && input.is_action_pressed[INPUT_UP] && ui_hover_index != -1 && input.mouse_raw_yrel != 0) {
            sectors[selected_sectors[ui_hover_index]].ceiling_y -= input.mouse_raw_yrel;
            if (sectors[selected_sectors[ui_hover_index]].ceiling_y <= sectors[selected_sectors[ui_hover_index]].floor_y) {
                sectors[selected_sectors[ui_hover_index]].ceiling_y = sectors[selected_sectors[ui_hover_index]].floor_y + 1;
            }
            changing_floor_or_ceiling = true;
        }

        // begin changing floor
        if (mode == MODE_SECTOR && input.is_action_pressed[INPUT_DOWN] && ui_hover_index != -1 && input.mouse_raw_yrel != 0) {
            sectors[selected_sectors[ui_hover_index]].floor_y -= input.mouse_raw_yrel;
            if (sectors[selected_sectors[ui_hover_index]].floor_y >= sectors[selected_sectors[ui_hover_index]].ceiling_y) {
                sectors[selected_sectors[ui_hover_index]].floor_y = sectors[selected_sectors[ui_hover_index]].ceiling_y - 1;
            }
            changing_floor_or_ceiling = true;
        }

        // end changing ceiling or floor
        if (mode == MODE_SECTOR && (input.is_action_just_released[INPUT_UP] || input.is_action_just_released[INPUT_DOWN]) && changing_floor_or_ceiling) {
            changing_floor_or_ceiling = false;
            level_init_sectors();
        }

        // delete sector
        if (mode == MODE_SECTOR && input.is_action_just_pressed[INPUT_DELETE] && ui_hover_index != -1) {
            sectors.erase(sectors.begin() + selected_sectors[ui_hover_index]);
            selected_sectors.erase(selected_sectors.begin() + ui_hover_index);
            ui_hover_index = -1;
            level_init_sectors();
            refresh_ui_boxes();
        }

        // toggle wall hidden
        if (mode == MODE_VERTEX && input.is_action_just_pressed[INPUT_FORWARD] && ui_hover_index != -1) {
            sectors[selected_sectors[0]].walls[ui_hover_index].exists = !sectors[selected_sectors[0]].walls[ui_hover_index].exists;
            level_init_sectors();
        }

        // change current texture
        if ((mode == MODE_SECTOR || mode == MODE_VERTEX) && is_mouse_in_rect(texture_picker_rect)) {
            if (input.is_action_just_pressed[INPUT_UP]) {
                current_texture = (current_texture + 1) % NUM_TEXTURES;
            } else if (input.is_action_just_pressed[INPUT_DOWN]) {
                current_texture = (current_texture + (NUM_TEXTURES - 1)) % NUM_TEXTURES;
            }
        }

        // set wall texture
        if (mode == MODE_VERTEX && ui_hover_index != -1 && input.is_action_just_pressed[INPUT_T]) {
            sectors[selected_sectors[0]].walls[ui_hover_index].texture_index = current_texture;
            level_init_sectors();
        }

        // set ceiling texture
        if (mode == MODE_SECTOR && ui_hover_index != -1 && input.is_action_just_pressed[INPUT_T]) {
            sectors[selected_sectors[ui_hover_index]].ceiling_texture_index = current_texture;
            level_init_sectors();
        }

        // set floor texture
        if (mode == MODE_SECTOR && ui_hover_index != -1 && input.is_action_just_pressed[INPUT_G]) {
            sectors[selected_sectors[ui_hover_index]].floor_texture_index = current_texture;
            level_init_sectors();
        }

        // begin changing object y
        if (mode == MODE_OBJECT && ui_hover_index != -1 && input.is_action_pressed[INPUT_UP]) {
            if (object_selections[ui_hover_index].type == OBJECT_PLAYER) {
                player_spawn_point.y -= input.mouse_raw_yrel * 0.5f;
            } else if (object_selections[ui_hover_index].type == OBJECT_ENEMY) {
                enemy_spawns[object_selections[ui_hover_index].index].position.y -= input.mouse_raw_yrel * 0.5f;
            }
        }

        // change enemy direction
        if (mode == MODE_OBJECT && ui_hover_index != -1 && object_selections[ui_hover_index].type == OBJECT_ENEMY && input.is_action_just_pressed[INPUT_RIGHT]) {
            const glm::vec2 directions[] = {
                glm::vec2(0.0f, 1.0f),
                glm::vec2(-1.0f, 0.0f),
                glm::vec2(0.0f, -1.0f),
                glm::vec2(1.0f, 0.0f)
            };

            for (unsigned int direction_index = 0; direction_index < 4; direction_index++) {
                if (enemy_spawns[object_selections[ui_hover_index].index].direction == directions[direction_index]) {
                    unsigned int new_direction_index = (direction_index + 1) % 4;
                    enemy_spawns[object_selections[ui_hover_index].index].direction = directions[new_direction_index];
                    break;
                }
            }
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
        bool i_is_selected = std::find(selected_sectors.begin(), selected_sectors.end(), i) != selected_sectors.end();
        if (i_is_selected) {
            continue;
        }

        edit_render_sector_walls(sectors[i], wall_color, hidden_wall_color);
    }

    // draw vertices
    for (unsigned int i = 0; i < sectors.size(); i++) {
        bool i_is_selected = std::find(selected_sectors.begin(), selected_sectors.end(), i) != selected_sectors.end();
        if (i_is_selected) {
            continue;
        }

        edit_render_sector_vertices(sectors[i], vertex_color);
    }

    if (mode == MODE_SECTOR || mode == MODE_VERTEX) {
        // selected sector walls
        for (unsigned int selected_sector : selected_sectors) {
            edit_render_sector_walls(sectors[selected_sector], selected_wall_color, selected_hidden_wall_color);
            edit_render_sector_vertices(sectors[selected_sector], selected_vertex_color);
        }
    }

    if (mode == MODE_NEW_SECTOR || mode == MODE_NEW_OBJECT) {
        SDL_SetRenderDrawColor(renderer, vertex_cursor_color.r, vertex_cursor_color.g, vertex_cursor_color.b, vertex_cursor_color.a);
        glm::ivec2 mouse_snapped_position = glm::ivec2(input.mouse_raw_x / 4, input.mouse_raw_y / 4) - camera_offset;
        SDL_Rect v = {
            .x = mouse_snapped_position.x + camera_offset.x,
            .y = mouse_snapped_position.y + camera_offset.y,
            .w = 1,
            .h = 1
        };
        SDL_RenderDrawRect(renderer, &v);
    }
    if (mode == MODE_NEW_SECTOR) {
        edit_render_sector_vertices(new_sector, vertex_color);
    }

    // Render player spawn
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (ObjectSelection& object_selection : object_selections) {
        if (object_selection.type == OBJECT_PLAYER) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            break;
        }
    }
    SDL_Rect p = {
        .x = (int)(player_spawn_point.x * 8.0f) + camera_offset.x,
        .y = (int)(player_spawn_point.z * 8.0f) + camera_offset.y,
        .w = 1,
        .h = 1
    };
    SDL_RenderDrawRect(renderer, &p);

    // Render enemy spawns
    for (unsigned int i = 0; i < enemy_spawns.size(); i++) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for (ObjectSelection& object_selection : object_selections) {
            if (object_selection.type == OBJECT_ENEMY && object_selection.index == i) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                break;
            }
        }

        p = {
            .x = (int)(enemy_spawns[i].position.x * 8.0f) + camera_offset.x,
            .y = (int)(enemy_spawns[i].position.z * 8.0f) + camera_offset.y,
            .w = 1,
            .h = 1
        };
        SDL_RenderDrawRect(renderer, &p);
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
    if (mode == MODE_SAVED_FILE) {
        edit_render_ui_text("Saved file");
    } else if (mode == MODE_SECTOR) {
        edit_render_ui_text("Sector Mode");
        for (unsigned int selected_sector : selected_sectors) {
            edit_render_ui_text("Sector " + std::to_string(selected_sector));
            edit_render_ui_text("ceil: " + std::to_string(sectors[selected_sector].ceiling_y));
            edit_render_ui_text("floor: " + std::to_string(sectors[selected_sector].floor_y));
            edit_render_ui_text("ceil tex: " + std::to_string(sectors[selected_sector].ceiling_texture_index));
            edit_render_ui_text("floor tex: " + std::to_string(sectors[selected_sector].floor_texture_index));
        }
    } else if (mode == MODE_VERTEX) {
        edit_render_ui_text("Vertex Mode");
        for (unsigned int i = 0; i < sectors[selected_sectors[0]].vertices.size(); i++) {
            edit_render_ui_text("Vertex " + std::to_string(i));
            std::string wall_value = "exists";
            if (!sectors[selected_sectors[0]].walls[i].exists) {
                wall_value = "hidden";
            }
            edit_render_ui_text("wall: " + wall_value);
            edit_render_ui_text("texture: " + std::to_string(sectors[selected_sectors[0]].walls[i].texture_index));
        }
    } else if (mode == MODE_NEW_SECTOR) {
        edit_render_ui_text("New Sector Mode");
    } else if (mode == MODE_OBJECT) {
        edit_render_ui_text("Object Mode");
        for (ObjectSelection& object_selection : object_selections) {
            if (object_selection.type == OBJECT_PLAYER) {
                edit_render_ui_text("Player");
                edit_render_ui_text("y: " + std::to_string(player_spawn_point.y));
            } else if (object_selection.type == OBJECT_ENEMY) {
                edit_render_ui_text("Enemy " + std::to_string(object_selection.index));
                edit_render_ui_text("y: " + std::to_string(enemy_spawns[object_selection.index].position.y));
                edit_render_ui_text("direction: " + std::to_string((int)enemy_spawns[object_selection.index].direction.x) + "," + std::to_string((int)enemy_spawns[object_selection.index].direction.y));
            }
        }
    } else if (mode == MODE_NEW_OBJECT) {
        edit_render_ui_text("New Object Mode");
        if (new_object_type == OBJECT_ENEMY) {
            edit_render_ui_text("Type: Enemy");
        }
    }

    // render texture
    SDL_Rect src_rect = {
        .x = 0,
        .y = 0,
        .w = 64,
        .h = 64
    };
    SDL_Rect dst_rect = {
        .x = ui_rect.x + 64,
        .y = (int)(viewport_height - 64),
        .w = 64,
        .h = 64
    };
    SDL_RenderCopy(renderer, textures[current_texture], &src_rect, &dst_rect);

    SDL_RenderPresent(renderer);
}

void edit_render_sector_walls(const Sector& sector, SDL_Color wall_color, SDL_Color non_wall_color) {
    for (unsigned int j = 0; j < sector.vertices.size(); j++) {
        SDL_Rect v = {
            .x = (int)(sector.vertices[j].x * 8.0f) + camera_offset.x,
            .y = (int)(sector.vertices[j].y * 8.0f) + camera_offset.y,
            .w = 1,
            .h = 1
        };
        unsigned int other_j = (j + 1) % sector.vertices.size();
        SDL_Rect v2 = {
            .x = (int)(sector.vertices[other_j].x * 8.0f) + camera_offset.x,
            .y = (int)(sector.vertices[other_j].y * 8.0f) + camera_offset.y,
            .w = 1,
            .h = 1
        };

        if (sector.walls[j].exists) {
            SDL_SetRenderDrawColor(renderer, wall_color.r, wall_color.g, wall_color.b, wall_color.a);
        } else {
            SDL_SetRenderDrawColor(renderer, non_wall_color.r, non_wall_color.g, non_wall_color.b, non_wall_color.a);
        }
        SDL_RenderDrawLine(renderer, v.x, v.y, v2.x, v2.y);
    }
}

void edit_render_sector_vertices(const Sector& sector, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (unsigned int j = 0; j < sector.vertices.size(); j++) {
        SDL_Rect v = {
            .x = (int)(sector.vertices[j].x * 8.0f) + camera_offset.x,
            .y = (int)(sector.vertices[j].y * 8.0f) + camera_offset.y,
            .w = 1,
            .h = 1
        };

        SDL_RenderDrawRect(renderer, &v);
    }
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
