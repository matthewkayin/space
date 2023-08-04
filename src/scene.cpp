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

struct BulletHole {
    glm::vec3 position;
    glm::vec3 normal;
};

Player player;

void scene_init() {
    glUseProgram(billboard_shader);
    glUniform1i(glGetUniformLocation(billboard_shader, "u_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUseProgram(ui_shader);
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUniform2iv(glGetUniformLocation(ui_shader, "screen_size"), 1, glm::value_ptr(screen_size));

    player.init();
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

    // render bullet holes
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


    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_bullet_hole);
    glBindVertexArray(quad_vao);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.0f, -1.0f));
    model = glm::scale(model, glm::vec3(0.1f));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

    player.render();
}
