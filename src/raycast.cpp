#include "raycast.hpp"

#include <map>
#include <cstdio>

std::vector<RaycastPlane> raycast_planes;

unsigned int raycast_add_plane(RaycastPlane plane) {
    raycast_planes.push_back(plane);

    return raycast_planes.size() - 1;
}

RaycastResult raycast_cast(glm::vec3 origin, glm::vec3 direction, float range, bool ignore_enemies) {
    // using multimap so that intersect distances are sorted in order of shortest to furthest distance
    std::multimap<float, unsigned int> intersect_distances;
    for (unsigned int plane = 0; plane < raycast_planes.size(); plane++) {
        const RaycastPlane& raycast_plane = raycast_planes[plane];

        if (!raycast_plane.enabled || (ignore_enemies && raycast_plane.type == PLANE_TYPE_ENEMY)) {
            continue;
        }

        // if normal and direction are perpendicular, then ray is parallel to plane
        if (glm::dot(direction, raycast_plane.normal) == 0.0f) {
            continue;
        }

        float nd = glm::dot(raycast_plane.normal, raycast_plane.a);
        float intersect_distance = (nd - glm::dot(origin, raycast_plane.normal)) / glm::dot(direction, raycast_plane.normal);

        // check that the plane is not "before" the ray origin
        if (intersect_distance < 0.0f) {
            continue;
        }

        if (intersect_distance <= range) {
            intersect_distances.insert(std::pair<float, unsigned int>(intersect_distance, plane));
        }
    }

    for (std::multimap<float, unsigned int>::iterator itr = intersect_distances.begin(); itr != intersect_distances.end(); ++itr) {
        const RaycastPlane& raycast_plane = raycast_planes[itr->second];
        glm::vec3 intersect_point = origin + (direction * itr->first);

        glm::vec3 b_minus_a = raycast_plane.b - raycast_plane.a;
        float a_dot_b_minus_a = glm::dot(raycast_plane.a, b_minus_a);
        float i_dot_b_minus_a = glm::dot(intersect_point, b_minus_a);
        float b_dot_b_minus_a = glm::dot(raycast_plane.b, b_minus_a);

        if (a_dot_b_minus_a > i_dot_b_minus_a || i_dot_b_minus_a > b_dot_b_minus_a) {
            continue;
        }

        glm::vec3 d_minus_a = raycast_plane.d - raycast_plane.a;
        float a_dot_d_minus_a = glm::dot(raycast_plane.a, d_minus_a);
        float i_dot_d_minus_a = glm::dot(intersect_point, d_minus_a);
        float d_dot_d_minus_a = glm::dot(raycast_plane.d, d_minus_a);

        if (a_dot_d_minus_a > i_dot_d_minus_a || i_dot_d_minus_a > d_dot_d_minus_a) {
            continue;
        }

        return {
            .hit = true,
            .plane = itr->second,
            .point = intersect_point,
        };
    }

    return {
        .hit = false,
        .plane = 0,
        .point = glm::vec3(0.0f, 0.0f, 0.0f)
    };
}

float raycast_cast2d(glm::vec2 a_origin, glm::vec2 a_direction, glm::vec2 b_origin, glm::vec2 b_direction) {
    glm::vec2 b_minus_a = b_origin - a_origin;
    float a_direction_cross_b_direction = (a_direction.x * b_direction.y) - (a_direction.y * b_direction.x);

    float a_time = ((b_minus_a.x * b_direction.y) - (b_minus_a.y * b_direction.x)) / a_direction_cross_b_direction;
    float b_time = ((b_minus_a.x * a_direction.y) - (b_minus_a.y * a_direction.x)) / a_direction_cross_b_direction;

    if (a_time < 0.0f || a_time > 1.0f || b_time < 0.0f || b_time > 1.0f) {
        return -1.0f;
    }

    return a_time;
}

