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

struct Sector {
    std::vector<glm::vec2> vertices;
    float floor_y;
    float ceiling_y;

    std::vector<glm::vec3> wall_normals;
    glm::vec2 aabb_top_left;
    glm::vec2 aabb_bot_right;

    std::vector<VertexData> vertex_data;
    unsigned int vertex_data_size;
    unsigned int vao, vbo;

    Sector();
    void init_buffers();
    glm::vec3 get_collision_normal(const glm::vec3& position) const;
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
