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
};

struct Sector {
    std::vector<glm::vec2> vertices;
    float floor_y;
    float ceiling_y;
    std::vector<VertexData> vertex_data;
    std::vector<CollisionTriangle> collision_triangles;
    unsigned int vao, vbo;

    Sector();
    void init_buffers();
    bool is_point_colliding(const glm::vec3 point) const;
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
