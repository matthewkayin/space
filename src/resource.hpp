#pragma once

#include <map>
#include <glm/glm.hpp>

extern unsigned int resource_textures;
extern unsigned int resource_player_pistol;
extern unsigned int resource_bullet_hole;
extern unsigned int resource_wasp;
extern unsigned int resource_wasp_bullet_hole;

extern std::map<unsigned int, glm::ivec2> resource_extents;

bool resource_load_all();
