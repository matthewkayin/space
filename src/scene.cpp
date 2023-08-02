#include "scene.hpp"

#include "globals.hpp"
#include "resource.hpp"
#include "level.hpp"
#include "player.hpp"
#include "shader.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Player player;

void scene_init() {
    glUseProgram(gun_shader);
    glUniform1i(glGetUniformLocation(gun_shader, "gun_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(gun_shader, "frame"), 0);
}

void scene_update(float delta) {
    player.update(delta);
    level_move_and_slide(&player.position, &player.velocity, delta);
}

void scene_render() {
    glm::vec3 player_direction = player.position - glm::vec3(player.basis[2]);
    glm::mat4 view;
    view = glm::lookAt(player.position, player_direction, glm::vec3(player.basis[1]));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 100.0f);

    level_render(view, projection, player.position, player.flashlight_direction, player.flashlight_on);

    glUseProgram(gun_shader);
    glUniform1ui(glGetUniformLocation(gun_shader, "flashlight_on"), player.flashlight_on);
    glUniform3fv(glGetUniformLocation(gun_shader, "view_pos"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(gun_shader, "player_direction"), 1, glm::value_ptr(player_direction));
    glUniform3fv(glGetUniformLocation(gun_shader, "player_flashlight.position"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(gun_shader, "player_flashlight.direction"), 1, glm::value_ptr(player.flashlight_direction));

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_player_pistol);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
