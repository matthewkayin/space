#include "player.hpp"

#include "level.hpp"
#include "input.hpp"
#include "resource.hpp"
#include "shader.hpp"
#include "globals.hpp"
#include "font.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum AnimationPistol {
    ANIMATION_PISTOL_IDLE,
    ANIMATION_PISTOL_FIRE,
    ANIMATION_PISTOL_RELOAD,
    ANIMATION_COUNT
};

const float ACCELERATION = 0.01f;
const float MAX_VELOCITY = 2.0f;
const glm::vec3 ROTATION_SPEED = glm::vec3(0.0075f, 0.0075f, 0.0075f);

glm::vec3 crosshair_color = glm::vec3(1.0f, 1.0f, 1.0f);
glm::ivec2 crosshair_extents = glm::ivec2(1, 3);
glm::ivec2 crosshair_sideways_extents = glm::ivec2(crosshair_extents.y, crosshair_extents.x);

void Player::init() {
    position = player_spawn_point;
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    basis = glm::mat4(1.0f);
    direction = -glm::vec3(basis[2]);
    flashlight_direction = -glm::vec3(basis[2]);
    flashlight_on = false;

    recoil = 0.0f;
    recoil_cooldown = 0.1f;

    clip_ammo = 9;
    clip_max = 9;
    reserve_ammo = 9 * 6;

    health = 100;
    max_health = 100;
    is_dead = false;

    animation.add_animation(ANIMATION_PISTOL_IDLE, {
        .start_frame = 0,
        .end_frame = 0,
        .frame_time = 0.0f
    });
    animation.add_animation(ANIMATION_PISTOL_FIRE, {
        .start_frame = 1,
        .end_frame = 4,
        .frame_time = 1.0f
    });
    animation.add_animation(ANIMATION_PISTOL_RELOAD, {
        .start_frame = 5,
        .end_frame = 30,
        .frame_time = 1.0f
    });

    glUseProgram(screen_shader);
    glUniform1ui(glGetUniformLocation(screen_shader, "player_health"), health);

    screen_animation = SCREEN_ANIMATION_NONE;
}

void Player::update(float delta) {
    // rotation input
    glm::vec3 rotation_input = glm::vec3(0.0f, -input.mouse_raw_yrel, -input.mouse_raw_xrel);
    if (input.is_action_pressed[INPUT_YAW_ROLL]) {
        rotation_input.x = rotation_input.z;
        rotation_input.z = 0;
    }

    // rotate!
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.x * ROTATION_SPEED.x * delta, glm::vec3(basis[2])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.y * ROTATION_SPEED.y * delta, glm::vec3(basis[0])) * basis;
    basis = glm::rotate(glm::mat4(1.0f), rotation_input.z * ROTATION_SPEED.z * delta, glm::vec3(basis[1])) * basis;

    // lerp flashlight
    if (input.is_action_just_pressed[INPUT_FLASHLIGHT]) {
        flashlight_on = !flashlight_on;
    }
    direction = direction + ((-glm::vec3(basis[2]) - direction) * 2.0f * delta);
    flashlight_direction = flashlight_direction + ((-glm::vec3(basis[2]) - flashlight_direction) * delta);

    // calculate acceleration
    glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    if (input.is_action_pressed[INPUT_FORWARD]) {
        acceleration -= glm::vec3(basis[2]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_BACKWARD]) {
        acceleration += glm::vec3(basis[2]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_RIGHT]) {
        acceleration += glm::vec3(basis[0]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_LEFT]) {
        acceleration -= glm::vec3(basis[0]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_UP]) {
        acceleration += glm::vec3(basis[1]) * ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_DOWN]) {
        acceleration -= glm::vec3(basis[1]) * ACCELERATION;
    }

    // update velocity
    velocity += acceleration * delta;
    if (glm::length(velocity) > MAX_VELOCITY) {
        velocity = glm::normalize(velocity) * MAX_VELOCITY;
    }

    // shoot
    // set result hit to false so that no bullet results are handled by the scene unless the player actually shot
    raycast_result.hit = false;
    if (input.is_action_just_pressed[INPUT_LCLICK]) {
        if (clip_ammo > 0 && (animation.animation == ANIMATION_PISTOL_IDLE || (animation.animation == ANIMATION_PISTOL_FIRE && animation.frame >= 2))) {
            animation.set_animation(ANIMATION_PISTOL_FIRE);

            glm::vec3 recoil_offset = glm::vec3(0.0f, 0.0f, 0.0f);
            if (recoil > 0.0f) {
                float recoil_strength = (1 + (rand() % (int)(100 * recoil))) / 100.0f;
                float drift_direction = 1.0f;
                if (rand() % 2 == 0) {
                    drift_direction = -1.0f;
                }
                recoil_offset = ((glm::vec3(basis[0]) * drift_direction) + glm::vec3(basis[1])) * recoil_strength * 0.5f;
            }
            glm::vec3 fire_point = position + direction + recoil_offset;
            glm::vec3 fire_direction = glm::normalize(fire_point - position);

            raycast_result = raycast_cast(position, fire_direction, 100.0f, false);

            clip_ammo--;
            recoil = std::min(recoil + 0.4f, 1.0f);
        }
    }

    // recoil cooldown
    recoil = std::max(0.0f, recoil - (recoil_cooldown * delta));

    // reload
    if (input.is_action_just_pressed[INPUT_RELOAD]) {
        if (animation.animation == ANIMATION_PISTOL_IDLE && clip_ammo < clip_max && reserve_ammo > 0) {
            animation.set_animation(ANIMATION_PISTOL_RELOAD);
        }
    }
    if (animation.animation == ANIMATION_PISTOL_RELOAD && clip_ammo < clip_max && animation.frame >= 22) {
        clip_ammo = std::min(clip_max, reserve_ammo);
        reserve_ammo -= clip_ammo;
    }

    // animation
    animation.update(delta);
    if (animation.is_finished) {
        if (animation.animation == ANIMATION_PISTOL_FIRE && clip_ammo == 0 && reserve_ammo > 0) {
            animation.set_animation(ANIMATION_PISTOL_RELOAD);
        } else {
            animation.set_animation(ANIMATION_PISTOL_IDLE);
        }
    }

    // screen animation
    elapsed += delta * (60.0f / 1000.0f);
    if (screen_animation != SCREEN_ANIMATION_NONE){
        screen_anim_timer += delta * (60.0f / 1000.0f);
        if (screen_animation == SCREEN_ANIMATION_FLASH && screen_anim_timer >= 0.1f) {
            screen_anim_timer = 0.0f;
            screen_animation = SCREEN_ANIMATION_NONE;
        }
    }
}

void Player::take_damage(unsigned int amount) {
    if (is_dead) {
        return;
    }
    health -= std::min(amount, health);
    glUseProgram(screen_shader);
    glUniform1ui(glGetUniformLocation(screen_shader, "player_health"), health);
    if (health <= 0) {
        is_dead = true;
        screen_animation = SCREEN_ANIMATION_FADE;
    } else if (screen_animation == SCREEN_ANIMATION_NONE) {
        screen_animation = SCREEN_ANIMATION_FLASH;
    }
    screen_anim_timer = 0.0f;
}

void Player::render() {
    // prepare billboard shader for gun
    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_player_pistol]));
    glm::mat4 unit_mat4 = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glm::vec3 normal = glm::normalize(glm::vec3(basis[2]));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), animation.frame);
    glUniform1ui(glGetUniformLocation(billboard_shader, "flip_h"), false);

    // render gun
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_player_pistol);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Render crosshair
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "extents"), 1, glm::value_ptr(crosshair_extents));
    glUniform3fv(glGetUniformLocation(ui_shader, "u_color"), 1, glm::value_ptr(crosshair_color));
    glBlendFunc(GL_ONE, GL_ZERO);
    glBindVertexArray(quad_vao);

    // top part
    glm::ivec2 crosshair_position = glm::ivec2(0, 8 + (int)(16.0f * recoil));
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

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    std::string health_string = "HEALTH: " + std::to_string(health) + "/" + std::to_string(max_health);
    font_hack_10pt.render_text(health_string, 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    std::string ammo_string = "AMMO: " + std::to_string(clip_ammo) + "/" + std::to_string(reserve_ammo);
    font_hack_10pt.render_text(ammo_string, 0.0f, 10.0f, glm::vec3(1.0f, 1.0f, 1.0f));
}
