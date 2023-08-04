#pragma once

#include <glm/glm.hpp>
#include <vector>

struct RaycastPlane {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 d;
    glm::vec3 normal;
};

struct RaycastResult {
    bool hit;
    glm::vec3 point;
};

extern std::vector<RaycastPlane> raycast_planes;

unsigned int raycast_add_plane(RaycastPlane plane);
RaycastResult raycast_cast(glm::vec3 origin, glm::vec3 direction, float range, unsigned int ignore);
