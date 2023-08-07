#include "enemy.hpp"

#include "raycast.hpp"
#include "resource.hpp"
#include "shader.hpp"
#include "globals.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum EnemyAnimation {
    ENEMY_ANIMATION_IDLE
};

EnemyBulletHole::EnemyBulletHole(glm::vec3 position, glm::vec3 normal) {
    this->position = position;
    this->normal = normal;

    animation.add_animation(0, {
        .start_frame = 0,
        .end_frame = 2,
        .frame_time = 2.0f
    });
    animation.set_animation(0);
}

void EnemyBulletHole::update(float delta) {
    if (animation.is_finished) {
        return;
    }

    animation.update(delta);
}

void EnemyBulletHole::render() {
    if (animation.is_finished) {
        return;
    }

    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_wasp_bullet_hole]));
    glm::mat4 model = glm::inverse(glm::lookAt(position, position + normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), false);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation.frame);

    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_wasp_bullet_hole);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

Enemy::Enemy() {
    position = glm::vec3(0.0f, 1.0f, -1.0f);
    direction = glm::vec3(0.0f, 0.0f, 1.0f);

    animation.add_animation(ENEMY_ANIMATION_IDLE, {
        .start_frame = 0,
        .end_frame = 2,
        .frame_time = 2.0f,
    });

    hurtbox_extents = glm::vec2((100.0f / 2.0f) / (SCREEN_WIDTH / 2), (120.0f / 2.0f) / (SCREEN_HEIGHT / 2));
    glm::vec3 facing_direction = direction;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 model = glm::inverse(glm::lookAt(position, position + facing_direction, up));
    hurtbox_raycast_plane = raycast_add_plane({
        .type = PLANE_TYPE_ENEMY,
        .id = 0,
        .a = glm::vec3(model * glm::vec4(-hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f)),
        .b = glm::vec3(model * glm::vec4(hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f)),
        .c = glm::vec3(model * glm::vec4(hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f)),
        .d = glm::vec3(model * glm::vec4(-hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f)),
        .normal = facing_direction
    });
}

void Enemy::update(float delta) {
    animation.update(delta);
}

void Enemy::render(glm::vec3 player_position) {
    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_wasp]));

    glm::vec3 facing_direction = glm::normalize(glm::vec3(player_position.x, position.y, player_position.z) - position);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(up, facing_direction)) == 1.0f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    glm::mat4 model = glm::inverse(glm::lookAt(position, position + facing_direction, up));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(facing_direction));

    float angle = atan2(direction.z - facing_direction.z, direction.x - facing_direction.x) * (180 / 3.14) * 2;
    if (angle > 180.0f) {
        angle = -(360.0f - angle);
    }
    animation_offset = (unsigned int)(abs(angle) / 36.0f);
    flip_h = angle < 0.0f && animation_offset >= 1 && animation_offset <= 3;

    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), flip_h);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation.frame + (animation_offset * 3));

    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_wasp);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // update hurtbox plane
    raycast_planes[hurtbox_raycast_plane].a = glm::vec3(model * glm::vec4(-hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].b = glm::vec3(model * glm::vec4(hurtbox_extents.x, -hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].c = glm::vec3(model * glm::vec4(hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].d = glm::vec3(model * glm::vec4(-hurtbox_extents.x, hurtbox_extents.y, 0.0f, 1.0f));
    raycast_planes[hurtbox_raycast_plane].normal = facing_direction;
}
