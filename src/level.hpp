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

struct Wall {
    bool exists;
    glm::vec3 normal;
};

struct Sector {
    std::vector<glm::vec2> vertices;
    float floor_y;
    float ceiling_y;

    std::vector<Wall> walls;
    glm::vec2 aabb_top_left;
    glm::vec2 aabb_bot_right;

    std::vector<VertexData> vertex_data;
    unsigned int vertex_data_size;
    unsigned int vao, vbo;

    Sector();
    void add_vertex(const glm::vec2 vertex, bool add_wall);
    void init_buffers();
    glm::vec3 get_collision_normal(const glm::vec3& position) const;
    void render(unsigned int shader);
};

struct Level {
    unsigned int texture_shader;
    unsigned int texture_array;

    std::vector<Sector> sectors;
    Player player;

    bool init();
    void update(float delta);
    void render();
};
