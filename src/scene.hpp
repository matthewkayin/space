#pragma once

#include <glm/glm.hpp>

void scene_init();
void scene_update(float delta);
void scene_move_and_slide(glm::vec3* position, glm::vec3* velocity, float delta);
void scene_render();
