#include "level.hpp"

#include "shader.hpp"
#include "input.hpp"
#include "globals.hpp"
#include "resource.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <cstdio>

std::vector<Sector> sectors;
std::vector<PointLight> lights;

float camera_yaw = -90.0f;
float camera_pitch = 0.0f;
glm::vec3 camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_position = glm::vec3(0.0f, 0.5f, 0.0f);

float raycast(glm::vec2 a_origin, glm::vec2 a_direction, glm::vec2 b_origin, glm::vec2 b_direction) {
    glm::vec2 b_minus_a = b_origin - a_origin;
    float a_direction_cross_b_direction = (a_direction.x * b_direction.y) - (a_direction.y * b_direction.x);

    float a_time = ((b_minus_a.x * b_direction.y) - (b_minus_a.y * b_direction.x)) / a_direction_cross_b_direction;
    float b_time = ((b_minus_a.x * a_direction.y) - (b_minus_a.y * a_direction.x)) / a_direction_cross_b_direction;

    if (a_time < 0.0f || a_time > 1.0f || b_time < 0.0f || b_time > 1.0f) {
        return -1.0f;
    }

    return a_time;
}

Sector::Sector() {
    has_generated_buffers = false;
    floor_y = 0.0f;
    ceiling_y = 1.0f;
    ceiling_texture_index = 0;
    floor_texture_index = 0;
}

Sector::~Sector() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Sector::add_vertex(const glm::vec2 vertex, unsigned int texture_index, bool wall_exists) {
    vertices.push_back(vertex);
    walls.push_back({
        .exists = wall_exists,
        .texture_index = texture_index,
        .normal = glm::vec3(0.0f, 0.0f, 0.0f)
    });
}

void Sector::init_buffers() {
    std::vector<VertexData> vertex_data;

    // walls
    for (unsigned int i = 0; i < vertices.size(); i++) {
        if (!walls[i].exists) {
            continue;
        }

        unsigned int end_index = (i + 1) % vertices.size();
        glm::vec3 wall_top_left = glm::vec3(vertices[i].x, ceiling_y, vertices[i].y);
        glm::vec3 wall_bot_left = glm::vec3(vertices[i].x, floor_y, vertices[i].y);
        glm::vec3 wall_top_right = glm::vec3(vertices[end_index].x, ceiling_y, vertices[end_index].y);
        glm::vec3 wall_bot_right = glm::vec3(vertices[end_index].x, floor_y, vertices[end_index].y);

        glm::vec3 wall_vertices[6] = {
            wall_top_left,
            wall_top_right,
            wall_bot_left,

            wall_top_right,
            wall_bot_right,
            wall_bot_left
        };

        glm::vec2 wall_scale = glm::vec2(glm::length(vertices[i] - vertices[end_index]), std::fabs(ceiling_y - floor_y));
        glm::vec2 texture_coordinates[6] = {
            glm::vec2(0.0f, wall_scale.y),
            glm::vec2(wall_scale.x, wall_scale.y),
            glm::vec2(0.0f, 0.0f),

            glm::vec2(wall_scale.x, wall_scale.y),
            glm::vec2(wall_scale.x, 0.0f),
            glm::vec2(0.0f, 0.0f)
        };

        for (unsigned int face = 0; face < 2; face++) {
            unsigned int base_index = face * 3;
            glm::vec3 face_normal = glm::normalize(glm::cross(wall_vertices[base_index + 2] - wall_vertices[base_index], wall_vertices[base_index + 1] - wall_vertices[base_index]));

            for (unsigned int j = 0; j < 3; j++) {
                vertex_data.push_back({
                    .position = wall_vertices[base_index + j],
                    .normal = face_normal,
                    .texture_index = walls[i].texture_index,
                    .texture_coordinates = texture_coordinates[base_index + j]
                });
            }

            if (face == 0) {
                walls[i].normal = face_normal;
            }
        }
    }

    // determine AABB
    aabb_top_left = vertices[0];
    aabb_bot_right = vertices[0];
    for (unsigned int i = 1; i < vertices.size(); i++) {
        aabb_top_left.x = std::min(aabb_top_left.x, vertices[i].x);
        aabb_top_left.y = std::min(aabb_top_left.y, vertices[i].y);
        aabb_bot_right.x = std::max(aabb_bot_right.x, vertices[i].x);
        aabb_bot_right.y = std::max(aabb_bot_right.y, vertices[i].y);
    }
    aabb[0] = glm::vec4(aabb_top_left.x, ceiling_y, aabb_top_left.y, 1.0f);
    aabb[1] = glm::vec4(aabb_bot_right.x, ceiling_y, aabb_top_left.y, 1.0f);
    aabb[2] = glm::vec4(aabb_top_left.x, ceiling_y, aabb_bot_right.y, 1.0f);
    aabb[3] = glm::vec4(aabb_bot_right.x, ceiling_y, aabb_bot_right.y, 1.0f);
    aabb[4] = glm::vec4(aabb_top_left.x, floor_y, aabb_top_left.y, 1.0f);
    aabb[5] = glm::vec4(aabb_bot_right.x, floor_y, aabb_top_left.y, 1.0f);
    aabb[6] = glm::vec4(aabb_top_left.x, floor_y, aabb_bot_right.y, 1.0f);
    aabb[7] = glm::vec4(aabb_bot_right.x, floor_y, aabb_bot_right.y, 1.0f);

    // ceiling and floor
    // first, divide ceiling polygon into triangles by looking for ear triangles
    std::vector<unsigned int> remaining_vertices;
    std::vector<glm::ivec3> ceiling_triangle_vertices;
    for (unsigned int i = 0; i < vertices.size(); i++) {
        remaining_vertices.push_back(i);
    }
    // when there are only three vertices remaining, we can break out of this loop and add the last three as a triangle
    while (remaining_vertices.size() > 3) {
        for (unsigned int i = 0; i < remaining_vertices.size(); i++) {
            unsigned int candidate_vertex = remaining_vertices[i];
            unsigned int left_vertex = remaining_vertices[(i + remaining_vertices.size() - 1) % remaining_vertices.size()];
            unsigned int right_vertex = remaining_vertices[(i + 1) % remaining_vertices.size()];

            // check that the candidate triangle is concave
            glm::vec2 left_vertex_vector = vertices[left_vertex] - vertices[candidate_vertex];
            glm::vec2 right_vertex_vector = vertices[right_vertex] - vertices[candidate_vertex];
            float angle = glm::degrees(acos(glm::dot(glm::normalize(left_vertex_vector), glm::normalize(right_vertex_vector))));

            if (angle >= 180) {
                continue;
            }

            // then check that no other points lie inside the candidate triangle
            glm::vec2 a = vertices[candidate_vertex];
            glm::vec2 b = vertices[left_vertex];
            glm::vec2 c = vertices[right_vertex];
            bool abc_is_valid_ear = true;
            for (unsigned int j = 0; j < remaining_vertices.size(); j++) {
                if (remaining_vertices[j] == candidate_vertex || remaining_vertices[j] == left_vertex || remaining_vertices[j] == right_vertex) {
                    continue;
                }

                glm::vec2 p = vertices[remaining_vertices[j]];
                float A = abs((a.x*(b.y - c.y) + b.x*(c.y - a.y) + c.x*(a.y - b.y)) / 2.0f);
                float A1 = abs((p.x*(b.y - c.y) + b.x*(c.y - p.y) + c.x*(p.y - b.y)) / 2.0f);
                float A2 = abs((a.x*(p.y - c.y) + p.x*(c.y - a.y) + c.x*(a.y - p.y)) / 2.0f);
                float A3 = abs((a.x*(b.y - p.y) + b.x*(p.y - a.y) + p.x*(a.y - b.y)) / 2.0f);

                bool is_p_inside_abc = A == A1 + A2 + A3;
                if (is_p_inside_abc) {
                    abc_is_valid_ear = false;
                    break;
                }
            }

            // if candidate is concave and contains no other points, it's valid and gets added to triangles list
            if (abc_is_valid_ear) {
                ceiling_triangle_vertices.push_back(glm::ivec3(candidate_vertex, right_vertex, left_vertex));
                remaining_vertices.erase(remaining_vertices.begin() + i);
                break;
            }
        }
    }
    // add the last triangle
    ceiling_triangle_vertices.push_back(glm::ivec3(remaining_vertices[0], remaining_vertices[1], remaining_vertices[2]));

    // make ceiling and floor triangles out of the triangles formed above
    glm::vec2 ceiling_scale = glm::vec2(std::fabs(aabb_bot_right.x - aabb_top_left.x), std::fabs(aabb_top_left.y - aabb_bot_right.y));
    for (glm::ivec3 ceiling_triangle : ceiling_triangle_vertices) {
        glm::vec3 triangle_vertices[3] = {
            glm::vec3(vertices[ceiling_triangle[0]].x, ceiling_y, vertices[ceiling_triangle[0]].y),
            glm::vec3(vertices[ceiling_triangle[1]].x, ceiling_y, vertices[ceiling_triangle[1]].y),
            glm::vec3(vertices[ceiling_triangle[2]].x, ceiling_y, vertices[ceiling_triangle[2]].y),
        };
        glm::vec3 face_normal = glm::normalize(glm::cross(triangle_vertices[1] - triangle_vertices[0], triangle_vertices[2] - triangle_vertices[0]));
        for (unsigned int j = 0; j < 3; j++) {
            vertex_data.push_back({
                .position = triangle_vertices[j],
                .normal = face_normal,
                .texture_index = ceiling_texture_index,
                .texture_coordinates = glm::vec2(
                        ((triangle_vertices[j].x - aabb_top_left.x) / ceiling_scale.x) * ceiling_scale.x,
                        (std::fabs(triangle_vertices[j].z - aabb_bot_right.y) / ceiling_scale.y) * ceiling_scale.y)
            });
        }

        for (unsigned int j = 0; j < 3; j++) {
            triangle_vertices[j].y = floor_y;
        }
        face_normal = glm::normalize(glm::cross(triangle_vertices[2] - triangle_vertices[0], triangle_vertices[1] - triangle_vertices[0]));
        for (unsigned int j = 0; j < 3; j++) {
            vertex_data.push_back({
                .position = triangle_vertices[j],
                .normal = face_normal,
                .texture_index = floor_texture_index,
                .texture_coordinates = glm::vec2(
                        ((triangle_vertices[j].x - aabb_top_left.x) / ceiling_scale.x) * ceiling_scale.x,
                        (std::fabs(triangle_vertices[j].z - aabb_bot_right.y) / ceiling_scale.y) * ceiling_scale.y)
            });
        }
    }

    // insert vertex data into buffers
    if (!has_generated_buffers) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(VertexData), &vertex_data[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(VertexData), (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)((6 * sizeof(float)) + sizeof(unsigned int)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    vertex_data_size = vertex_data.size();
}

void Sector::render() {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertex_data_size);
    glBindVertexArray(0);
}

Frustum::Frustum(const glm::mat4& projection_view_transpose) {
    plane[0] = glm::vec4(projection_view_transpose[3] + projection_view_transpose[0]); // left
    plane[1] = glm::vec4(projection_view_transpose[3] - projection_view_transpose[0]); // right
    plane[2] = glm::vec4(projection_view_transpose[3] + projection_view_transpose[1]); // bottom
    plane[3] = glm::vec4(projection_view_transpose[3] - projection_view_transpose[1]); // top
    plane[4] = glm::vec4(projection_view_transpose[3] + projection_view_transpose[2]); // near
    plane[5] = glm::vec4(projection_view_transpose[3] - projection_view_transpose[2]); // far
}

bool Frustum::is_inside(const Sector& sector) const {
    for (unsigned int plane_index = 0; plane_index < 6; plane_index++) {
        bool all_inside_plane_halfspace = true;
        for (unsigned int aabb_index = 0; aabb_index < 8; aabb_index++) {
            if (glm::dot(sector.aabb[aabb_index], plane[plane_index]) >= 0.0f) {
                all_inside_plane_halfspace = false;
                break;
            }
        }
        if (all_inside_plane_halfspace) {
            return false;
        }
    }

    return true;
}

void level_init() {
    // create sectors
    Sector a;
    a.add_vertex(glm::vec2(-3.0f, -1.0f), 0, true);
    a.add_vertex(glm::vec2(0.0f, -5.0f), 0, false);
    a.add_vertex(glm::vec2(3.0f, -1.0f), 0, true);
    a.add_vertex(glm::vec2(3.0f, 5.0f), 0, true);
    a.add_vertex(glm::vec2(-3.0f, 5.0f), 0, true);
    a.floor_y = 0.0f;
    a.ceiling_y = 3.0f;
    sectors.push_back(a);

    Sector b;
    b.add_vertex(glm::vec2(3.0f, -9.0f), 0, true);
    b.add_vertex(glm::vec2(6.0f, -5.0f), 0, true);
    b.add_vertex(glm::vec2(3.0f, -1.0f), 0, false);
    b.add_vertex(glm::vec2(0.0f, -5.0f), 0, true);
    b.floor_y = 0.0f;
    b.ceiling_y = 3.0f;
    sectors.push_back(b);

    // create lights
    PointLight light = {
        .position = glm::vec3(-2.8, 1.5f, 4.8f),
        .constant = 1.0f,
        .linear = 0.022f,
        .quadratic = 0.0019f
    };
    PointLight light2 = {
        .position = glm::vec3(1.0f, 1.5f, -2.0f),
        .constant = 1.0f,
        .linear = 0.022f,
        .quadratic = 0.0019f
    };
    lights.push_back(light);

    glUseProgram(texture_shader);
    glUniform1i(glGetUniformLocation(texture_shader, "texture_array"), 0);
    glUniform1ui(glGetUniformLocation(texture_shader, "lighting_enabled"), !edit_mode);

    unsigned int shaders_with_lighting[] = { texture_shader, gun_shader };

    for (unsigned int shader_index = 0; shader_index < 2; shader_index++) {
        unsigned int shader = shaders_with_lighting[shader_index];
        glUseProgram(shader);

        glUniform1ui(glGetUniformLocation(shader, "point_light_count"), lights.size());
        for (unsigned int i = 0; i < lights.size(); i++) {
            std::string shader_var_name = "point_lights[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader, (shader_var_name + ".position").c_str()), 1, glm::value_ptr(lights[i].position));
            glUniform1f(glGetUniformLocation(shader, (shader_var_name + ".constant").c_str()), lights[i].constant);
            glUniform1f(glGetUniformLocation(shader, (shader_var_name + ".linear").c_str()), lights[i].linear);
            glUniform1f(glGetUniformLocation(shader, (shader_var_name + ".quadratic").c_str()), lights[i].quadratic);
        }

        glUniform1f(glGetUniformLocation(shader, "player_flashlight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(shader, "player_flashlight.linear"), 0.09);
        glUniform1f(glGetUniformLocation(shader, "player_flashlight.quadratic"), 0.032f);
        glUniform1f(glGetUniformLocation(shader, "player_flashlight.cutoff"), glm::cos(glm::radians(12.5f)));
        glUniform1f(glGetUniformLocation(shader, "player_flashlight.outer_cutoff"), glm::cos(glm::radians(17.5f)));
    }


    level_init_sectors();
}

void level_init_sectors() {
    for (unsigned int i = 0; i < sectors.size(); i++) {
        sectors[i].init_buffers();
    }
}

void level_move_and_slide(glm::vec3* position, glm::vec3* velocity, float delta) {
    // check collisions and move player
    if (glm::length(*velocity) == 0.0f) {
        return;
    }
    glm::vec3 actual_velocity = *velocity * delta;

    // check if within sector AABB
    std::vector<Sector*> nearby_sectors;
    for (unsigned int i = 0; i < sectors.size(); i++) {
        Sector* sector = &sectors[i];
        float padding = 1.0f;
        if (position->x < sector->aabb_top_left.x - padding || position->x > sector->aabb_bot_right.x + padding ||
            position->y < sector->floor_y || position->y > sector->ceiling_y ||
            position->z < sector->aabb_top_left.y - padding || position->z > sector->aabb_bot_right.y + padding) {
            continue;
        }
        nearby_sectors.push_back(sector);
    }

    // check floor / ceiling collisions
    glm::vec2 origin2d = glm::vec2(position->x, position->z);
    for (Sector* sector : nearby_sectors) {
        glm::vec2 raycast_start = sector->aabb_top_left - glm::vec2(1.0f, 1.0f);
        glm::vec2 raycast_direction = origin2d - raycast_start;
        unsigned int hits = 0;
        for (unsigned int wall = 0; wall < sector->vertices.size(); wall++) {
            float raycast_result = raycast(raycast_start, raycast_direction, sector->vertices[wall], sector->vertices[(wall + 1) % sector->vertices.size()] - sector->vertices[wall]);
            if (raycast_result != -1.0f) {
                hits++;
            }
        }

        if (hits % 2 == 0) {
            continue;
        }

        float predicted_y = position->y + actual_velocity.y;
        if (predicted_y + 0.5f >= sector->ceiling_y) {
            glm::vec3 velocity_in_ceiling_normal_direction = glm::vec3(0.0f, -1.0f, 0.0f) * glm::dot(actual_velocity, glm::vec3(0.0f, -1.0f, 0.0f));
            *velocity -= velocity_in_ceiling_normal_direction;
            actual_velocity -= velocity_in_ceiling_normal_direction;
        } else if (predicted_y - 0.5f <= sector->floor_y) {
            glm::vec3 velocity_in_floor_normal_direction = glm::vec3(0.0f, 1.0f, 0.0f) * glm::dot(actual_velocity, glm::vec3(0.0f, 1.0f, 0.0f));
            *velocity -= velocity_in_floor_normal_direction;
            actual_velocity -= velocity_in_floor_normal_direction;
        }
    }

    // check wall collisions
    bool collided = true;
    unsigned int attempts = 0;
    while (collided && attempts < 10) {
        collided = false;
        attempts++;

        for (Sector* sector : nearby_sectors) {
            for (unsigned int wall = 0; wall < sector->vertices.size(); wall++) {
                if (!sector->walls[wall].exists) {
                    continue;
                }

                glm::vec2 velocity2d = glm::vec2(actual_velocity.x, actual_velocity.z);
                glm::vec2 predicted_origin2d = origin2d + velocity2d;

                glm::vec2 wallv = sector->vertices[(wall + 1) % sector->vertices.size()] - sector->vertices[wall];
                glm::vec2 f = sector->vertices[wall] - predicted_origin2d;
                float a = glm::dot(wallv, wallv);
                float b = 2 * glm::dot(f, wallv);
                float c = glm::dot(f, f) - 0.25f;
                float discriminant = (b * b) - (4.0f * a * c);

                if (discriminant < 0.0f) {
                    continue;
                }

                discriminant = sqrt(discriminant);
                float t1 = (-b - discriminant) / (2.0f * a);
                float t2 = (-b + discriminant) / (2.0f * a);

                if (!(t1 >= 0.0f && t1 <= 1.0f) && !(t2 >= 0.0f && t2 <= 1.0f)) {
                    continue;
                }

                glm::vec3 velocity_in_wall_normal_direction = sector->walls[wall].normal * glm::dot(actual_velocity, sector->walls[wall].normal);
                *velocity -= velocity_in_wall_normal_direction;
                actual_velocity = actual_velocity - velocity_in_wall_normal_direction;
                collided = true;
            }
        }
    }

    if (attempts == 5) {
        actual_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    *position += actual_velocity;
}

void level_edit_update(float delta) {
    camera_yaw += input.mouse_raw_xrel * 0.1f;
    camera_pitch -= input.mouse_raw_yrel * 0.1f;
    camera_pitch = std::min(std::max(camera_pitch, -89.0f), 89.0f);
    camera_direction = glm::normalize(glm::vec3(
                    cos(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch)),
                    sin(glm::radians(camera_pitch)),
                    sin(glm::radians(camera_yaw)) * cos(glm::radians(camera_pitch))));

    glm::vec3 camera_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    float camera_speed = 0.2f;
    if (input.is_action_pressed[INPUT_FORWARD]) {
        camera_velocity += camera_direction;
    }
    if (input.is_action_pressed[INPUT_BACKWARD]) {
        camera_velocity += -camera_direction;
    }
    if (input.is_action_pressed[INPUT_UP]) {
        camera_velocity += glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (input.is_action_pressed[INPUT_DOWN]) {
        camera_velocity += glm::vec3(0.0f, -1.0f, 0.0f);
    }
    if (input.is_action_pressed[INPUT_LEFT]) {
        camera_velocity += -glm::normalize(glm::cross(camera_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    if (input.is_action_pressed[INPUT_RIGHT]) {
        camera_velocity += glm::normalize(glm::cross(camera_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    }
    if (glm::length(camera_velocity) > 1.0f) {
        camera_velocity = glm::normalize(camera_velocity);
    }

    camera_position += camera_velocity * camera_speed * delta;
}

void level_render(glm::mat4 view, glm::mat4 projection, glm::vec3 view_pos, glm::vec3 flashlight_direction, bool flashlight_on) {
    glUseProgram(texture_shader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_textures);

    // view / projection transformations
    /*if (!edit_mode) {
    } else {
        view = glm::lookAt(camera_position, camera_position + camera_direction, glm::vec3(0.0f, 1.0f, 0.0f));
    }*/

    glUniform1ui(glGetUniformLocation(texture_shader, "flashlight_on"), flashlight_on);
    glUniformMatrix4fv(glGetUniformLocation(texture_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(texture_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(texture_shader, "view_pos"), 1, glm::value_ptr(view_pos));
    glUniform3fv(glGetUniformLocation(texture_shader, "player_flashlight.position"), 1, glm::value_ptr(view_pos));
    glUniform3fv(glGetUniformLocation(texture_shader, "player_flashlight.direction"), 1, glm::value_ptr(flashlight_direction));

    glm::mat4 projection_view_transpose = glm::transpose(projection * view);
    Frustum frustum = Frustum(projection_view_transpose);
    for (unsigned int i = 0; i < sectors.size(); i++) {
        if (!frustum.is_inside(sectors[i])) {
            continue;
        }

        sectors[i].render();
    }

}
