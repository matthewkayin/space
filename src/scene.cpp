#include "scene.hpp"

#include "player.hpp"
#include "globals.hpp"
#include "resource.hpp"
#include "level.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct BulletHole {
    glm::vec3 position;
    glm::vec3 normal;
};

Player player;
std::vector<BulletHole> bullet_holes;

void scene_init() {
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUseProgram(billboard_shader);
    glUniform1i(glGetUniformLocation(billboard_shader, "u_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUniform2iv(glGetUniformLocation(billboard_shader, "screen_size"), 1, glm::value_ptr(screen_size));
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "screen_size"), 1, glm::value_ptr(screen_size));

    player.init();
}

void scene_update(float delta) {
    player.update(delta);
    level_move_and_slide(&player.position, &player.velocity, delta);
    if (player.raycast_result.hit) {
        bullet_holes.push_back({
            .position = player.raycast_result.point + (player.raycast_result.normal * 0.05f),
            .normal = player.raycast_result.normal
        });
    }
}

void scene_render() {
    glm::vec3 camera_direction = player.position + player.direction;
    glm::mat4 view;
    view = glm::lookAt(player.position, camera_direction, glm::vec3(player.basis[1]));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 100.0f);

    level_render(view, projection, player.position, player.flashlight_direction, player.flashlight_on);

    // prepare billboard shader
    glUseProgram(billboard_shader);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flashlight_on"), player.flashlight_on);
    glUniform3fv(glGetUniformLocation(billboard_shader, "view_pos"), 1, glm::value_ptr(player.position));
    glm::vec3 normal = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.position"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.direction"), 1, glm::value_ptr(player.flashlight_direction));
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);

    // bind quad vertex
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_bullet_hole);
    glBindVertexArray(quad_vao);

    // render bullet holes
    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_bullet_hole]));
    for (const BulletHole& bullet_hole : bullet_holes) {
        glm::vec3 bullet_hole_up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(bullet_hole_up, bullet_hole.normal)) == 1.0f) {
            bullet_hole_up = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        glm::mat4 model = glm::inverse(glm::lookAt(bullet_hole.position, bullet_hole.position - bullet_hole.normal, bullet_hole_up));
        glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        normal = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
        glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    player.render();
}
