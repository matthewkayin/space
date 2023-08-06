#include "enemy.hpp"

#include "resource.hpp"
#include "shader.hpp"
#include "globals.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum EnemyAnimation {
    ENEMY_ANIMATION_IDLE
};

Enemy::Enemy() {
    position = glm::vec3(0.0f, 1.0f, -1.0f);
    direction = glm::vec3(0.0f, 0.0f, 1.0f);

    animation.add_animation(ENEMY_ANIMATION_IDLE, {
        .start_frame = 0,
        .end_frame = 2,
        .frame_time = 2.0f,
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
    unsigned int animation_offset = (unsigned int)(abs(angle) / 36.0f);
    bool flip_h = angle < 0.0f && animation_offset >= 1 && animation_offset <= 3;

    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), flip_h);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation.frame + (animation_offset * 3));

    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_wasp);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
