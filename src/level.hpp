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
    unsigned int texture_index;
    glm::vec3 normal;
};

struct PointLight {
    glm::vec3 position;

    float constant;
    float linear;
    float quadratic;
};

struct Sector {
    std::vector<glm::vec2> vertices;
    float floor_y;
    float ceiling_y;
    unsigned int floor_texture_index;
    unsigned int ceiling_texture_index;

    std::vector<Wall> walls;
    glm::vec2 aabb_top_left;
    glm::vec2 aabb_bot_right;
    glm::vec4 aabb[8];

    bool has_generated_buffers;
    unsigned int vao, vbo;
    unsigned int vertex_data_size;

    Sector();
    ~Sector();
    void add_vertex(const glm::vec2 vertex, unsigned int texture_index, bool wall_exists);
    void init_buffers();
    void render();
};

struct Frustum {
    glm::vec4 plane[6];
    Frustum(const glm::mat4& projection_view_transpose);
    bool is_inside(const Sector& sector) const;
};

extern std::vector<Sector> sectors;
extern std::vector<PointLight> lights;
extern Player player;

bool level_init();
void level_init_sectors();
void level_update(float delta);
void level_edit_update(float delta);
void level_render();
