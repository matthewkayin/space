#pragma once

#include "player.hpp"

#include <glm/glm.hpp>
#include <vector>

struct VertexData {
    glm::vec3 position;
    glm::vec3 normal;
    unsigned int texture_index;
    glm::vec2 texture_coordinates;
};

struct CollisionTriangle {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;
    glm::vec3 normal;
    glm::vec3 unit_u;
    glm::vec3 unit_v;
    float w;
    float g;
    float h;
    CollisionTriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 p_normal);
};

struct Sector {
    std::vector<glm::vec2> vertices;
    unsigned int vertex_data_size;
    float floor_y;
    float ceiling_y;
    std::vector<VertexData> vertex_data;
    std::vector<CollisionTriangle> collision_triangles;
    unsigned int vao, vbo;

    Sector();
    void init_buffers();
    float collision_check(const glm::vec3 point, const glm::vec3 velocity) const;
    void render(unsigned int shader);
};

struct Level {
    unsigned int texture_shader;
    unsigned int texture_array;

    Sector sector;
    Player player;

    bool init();
    void update(float delta);
    void render();
};
