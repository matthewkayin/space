#pragma once

#include <glm/glm.hpp>
#include <vector>

enum RaycastPlaneType {
    PLANE_TYPE_LEVEL,
    PLANE_TYPE_ENEMY
};

struct RaycastPlane {
    RaycastPlaneType type;
    unsigned int id; // what id is differs based on context, could be enemy id or sector id
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 d;
    glm::vec3 normal;
    bool enabled;
};

struct RaycastResult {
    bool hit;
    unsigned int plane;
    glm::vec3 point;
};

extern std::vector<RaycastPlane> raycast_planes;

unsigned int raycast_add_plane(RaycastPlane plane);
RaycastResult raycast_cast(glm::vec3 origin, glm::vec3 direction, float range, bool ignore_enemies);
float raycast_cast2d(glm::vec2 a_origin, glm::vec2 a_direction, glm::vec2 b_origin, glm::vec2 b_direction);
