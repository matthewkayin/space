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

glm::vec3 crosshair_color = glm::vec3(1.0f, 1.0f, 1.0f);
glm::ivec2 crosshair_extents = glm::ivec2(1, 3);
glm::ivec2 crosshair_sideways_extents = glm::ivec2(crosshair_extents.y, crosshair_extents.x);

void scene_init() {
    glUseProgram(gun_shader);
    glUniform1i(glGetUniformLocation(gun_shader, "gun_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(gun_shader, "frame"), 0);
    glUseProgram(ui_shader);
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform2iv(glGetUniformLocation(ui_shader, "screen_size"), 1, glm::value_ptr(screen_size));
}

void scene_update(float delta) {
    player.update(delta);
    level_move_and_slide(&player.position, &player.velocity, delta);
}

void scene_render() {
    glm::vec3 player_direction = player.position - player.direction;
    glm::mat4 view;
    view = glm::lookAt(player.position, player_direction, glm::vec3(player.basis[1]));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 100.0f);

    level_render(view, projection, player.position, player.flashlight_direction, player.flashlight_on);

    // Render player gun
    glUseProgram(gun_shader);
    glUniform1ui(glGetUniformLocation(gun_shader, "flashlight_on"), player.flashlight_on);
    glUniform3fv(glGetUniformLocation(gun_shader, "view_pos"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(gun_shader, "player_direction"), 1, glm::value_ptr(player_direction));
    glUniform3fv(glGetUniformLocation(gun_shader, "player_flashlight.position"), 1, glm::value_ptr(player.position));
    glUniform3fv(glGetUniformLocation(gun_shader, "player_flashlight.direction"), 1, glm::value_ptr(player.flashlight_direction));

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_player_pistol);
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Render crosshair
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "extents"), 1, glm::value_ptr(crosshair_extents));
    glUniform3fv(glGetUniformLocation(ui_shader, "u_color"), 1, glm::value_ptr(crosshair_color));
    glBlendFunc(GL_ONE, GL_ZERO);
    glBindVertexArray(quad_vao);

    // top part
    glm::ivec2 crosshair_position = glm::ivec2(0, 8);
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // bottom part
    crosshair_position.y *= -1;
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // left part
    glUniform2iv(glGetUniformLocation(ui_shader, "extents"), 1, glm::value_ptr(crosshair_sideways_extents));
    crosshair_position = glm::ivec2(crosshair_position.y, crosshair_position.x);
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // right part
    crosshair_position.x *= -1;
    glUniform2iv(glGetUniformLocation(ui_shader, "position"), 1, glm::value_ptr(crosshair_position));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}
