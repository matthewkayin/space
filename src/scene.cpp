#include "scene.hpp"

#include "font.hpp"
#include "globals.hpp"
#include "resource.hpp"
#include "level.hpp"
#include "shader.hpp"
#include "input.hpp"
#include "raycast.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const float PLAYER_ACCELERATION = 0.01f;
const float PLAYER_MAX_VELOCITY = 2.0f;
const glm::vec3 PLAYER_ROTATION_SPEED = glm::vec3(0.0075f, 0.0075f, 0.0075f);

glm::vec3 crosshair_color = glm::vec3(1.0f, 1.0f, 1.0f);
glm::ivec2 crosshair_extents = glm::ivec2(1, 3);
glm::ivec2 crosshair_sideways_extents = glm::ivec2(crosshair_extents.y, crosshair_extents.x);

enum AnimationPistol {
    ANIMATION_PISTOL_IDLE,
    ANIMATION_PISTOL_FIRE,
    ANIMATION_PISTOL_RELOAD,
    ANIMATION_COUNT
};

struct AnimationInfo {
    unsigned int texture_array;
    unsigned int start_frame;
    unsigned int end_frame;
    float frame_time;
};

struct Animation {
    unsigned int frame;
    unsigned int animation;
    float timer;
    bool is_finished;
    AnimationInfo animation_info[ANIMATION_COUNT];

    Animation() {
        frame = 0;
        animation = 0;
        timer = 0.0f;
        is_finished = false;
    }

    void set_animation(unsigned int animation) {
        this->animation = animation;
        frame = animation_info[animation].start_frame;
        timer = 0.0f;
    }

    void update(float delta) {
        is_finished = false;
        if (animation_info[animation].start_frame == animation_info[animation].end_frame) {
            return;
        }

        timer += delta;
        while (timer >= animation_info[animation].frame_time) {
            timer -= animation_info[animation].frame_time;
            frame++;
            if (frame > animation_info[animation].end_frame) {
                frame = animation_info[animation].start_frame;
                is_finished = true;
            }
        }
    }
};

struct BulletHole {
    glm::vec3 position;
    glm::vec3 normal;
};

glm::vec3 player_position;
glm::vec3 player_velocity;
glm::mat4 player_basis;
glm::vec3 player_direction;
glm::vec3 player_flashlight_direction;
bool player_flashlight_on;
Animation player_animation;
unsigned int player_reserve_ammo = 9 * 6;
unsigned int player_clip_ammo = 9;
unsigned int player_clip_max = 9;
float player_recoil = 0.0f;
float player_recoil_cooldown = 0.1f;

std::vector<BulletHole> bullet_holes;

void scene_init() {
    glm::ivec2 screen_size = glm::ivec2(SCREEN_WIDTH, SCREEN_HEIGHT);
    glUseProgram(billboard_shader);
    glUniform1i(glGetUniformLocation(billboard_shader, "u_texture_array"), 0);
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), 0);
    glUniform2iv(glGetUniformLocation(billboard_shader, "screen_size"), 1, glm::value_ptr(screen_size));
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "screen_size"), 1, glm::value_ptr(screen_size));

    player_position = glm::vec3(0.0f, 1.0f, 0.0f);
    player_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    player_basis = glm::mat4(1.0f);
    player_direction = glm::vec3(player_basis[2]);
    player_flashlight_direction = -glm::vec3(player_basis[2]);
    player_flashlight_on = false;

    player_animation.animation_info[ANIMATION_PISTOL_IDLE] = {
        .texture_array = resource_player_pistol,
        .start_frame = 0,
        .end_frame = 0,
        .frame_time = 0.0f,
    };
    player_animation.animation_info[ANIMATION_PISTOL_FIRE] = {
        .texture_array = resource_player_pistol,
        .start_frame = 1,
        .end_frame = 4,
        .frame_time = 1.0f,
    };
    player_animation.animation_info[ANIMATION_PISTOL_RELOAD] = {
        .texture_array = resource_player_pistol,
        .start_frame = 5,
        .end_frame = 30,
        .frame_time = 1.0f
    };
}

void scene_update(float delta) {
    // rotation input
    glm::vec3 rotation_input = glm::vec3(0.0f, -input.mouse_raw_yrel, -input.mouse_raw_xrel);
    if (input.is_action_pressed[INPUT_YAW_ROLL]) {
        rotation_input.x = rotation_input.z;
        rotation_input.z = 0;
    }

    // rotate!
    player_basis = glm::rotate(glm::mat4(1.0f), rotation_input.x * PLAYER_ROTATION_SPEED.x * delta, glm::vec3(player_basis[2])) * player_basis;
    player_basis = glm::rotate(glm::mat4(1.0f), rotation_input.y * PLAYER_ROTATION_SPEED.y * delta, glm::vec3(player_basis[0])) * player_basis;
    player_basis = glm::rotate(glm::mat4(1.0f), rotation_input.z * PLAYER_ROTATION_SPEED.z * delta, glm::vec3(player_basis[1])) * player_basis;

    // lerp flashlight
    if (input.is_action_just_pressed[INPUT_FLASHLIGHT]) {
        player_flashlight_on = !player_flashlight_on;
    }
    player_direction = player_direction + ((glm::vec3(player_basis[2]) - player_direction) * 2.0f * delta);
    player_flashlight_direction = player_flashlight_direction + ((-glm::vec3(player_basis[2]) - player_flashlight_direction) * delta);

    // calculate acceleration
    glm::vec3 player_acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    if (input.is_action_pressed[INPUT_FORWARD]) {
        player_acceleration -= glm::vec3(player_basis[2]) * PLAYER_ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_BACKWARD]) {
        player_acceleration += glm::vec3(player_basis[2]) * PLAYER_ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_RIGHT]) {
        player_acceleration += glm::vec3(player_basis[0]) * PLAYER_ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_LEFT]) {
        player_acceleration -= glm::vec3(player_basis[0]) * PLAYER_ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_UP]) {
        player_acceleration += glm::vec3(player_basis[1]) * PLAYER_ACCELERATION;
    }
    if (input.is_action_pressed[INPUT_DOWN]) {
        player_acceleration -= glm::vec3(player_basis[1]) * PLAYER_ACCELERATION;
    }

    // update velocity
    player_velocity += player_acceleration * delta;
    if (glm::length(player_velocity) > PLAYER_MAX_VELOCITY) {
        player_velocity = glm::normalize(player_velocity) * PLAYER_MAX_VELOCITY;
    }

    // shoot
    if (input.is_action_just_pressed[INPUT_LCLICK]) {
        if (player_clip_ammo > 0 && (player_animation.animation == ANIMATION_PISTOL_IDLE || (player_animation.animation == ANIMATION_PISTOL_FIRE && player_animation.frame >= 2))) {
            player_animation.set_animation(ANIMATION_PISTOL_FIRE);

            glm::vec3 recoil = glm::vec3(0.0f, 0.0f, 0.0f);
            if (player_recoil > 0.0f) {
                float recoil_strength = (1 + (rand() % (int)(100 * player_recoil))) / 100.0f;
                float drift_direction = 1.0f;
                if (rand() % 2 == 0) {
                    drift_direction = -1.0f;
                }
                recoil = ((glm::vec3(player_basis[0]) * drift_direction) + glm::vec3(player_basis[1])) * recoil_strength * 0.5f;
            }
            glm::vec3 player_fire_point = player_position - player_direction + recoil;
            glm::vec3 fire_direction = glm::normalize(player_fire_point - player_position);

            RaycastResult result = raycast_cast(player_position, fire_direction, 100.0f, 100);
            if (result.hit) {
                bullet_holes.push_back({
                    .position = result.point + (result.normal * 0.05f),
                    .normal = result.normal
                });
            }

            player_clip_ammo--;
            player_recoil = std::min(player_recoil + 0.4f, 1.0f);
        }
    }

    // recoil cooldown
    player_recoil = std::max(0.0f, player_recoil - (player_recoil_cooldown * delta));

    // reload
    if (input.is_action_just_pressed[INPUT_RELOAD]) {
        if (player_animation.animation == ANIMATION_PISTOL_IDLE && player_clip_ammo < player_clip_max && player_reserve_ammo > 0) {
            player_animation.set_animation(ANIMATION_PISTOL_RELOAD);
        }
    }
    if (player_animation.animation == ANIMATION_PISTOL_RELOAD && player_clip_ammo < player_clip_max && player_animation.frame >= 22) {
        player_clip_ammo = std::min(player_clip_max, player_reserve_ammo);
        player_reserve_ammo -= player_clip_ammo;
    }

    // player animation
    player_animation.update(delta);

    if (player_animation.is_finished) {
        if (player_animation.animation == ANIMATION_PISTOL_FIRE && player_clip_ammo == 0 && player_reserve_ammo > 0) {
            player_animation.set_animation(ANIMATION_PISTOL_RELOAD);
        } else {
            player_animation.set_animation(ANIMATION_PISTOL_IDLE);
        }
    }

    level_move_and_slide(&player_position, &player_velocity, delta);
}

void scene_render() {
    glm::vec3 camera_direction = player_position - player_direction;
    glm::mat4 view;
    view = glm::lookAt(player_position, camera_direction, glm::vec3(player_basis[1]));
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 100.0f);

    level_render(view, projection, player_position, player_flashlight_direction, player_flashlight_on);

    // prepare billboard shader
    glUseProgram(billboard_shader);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform1ui(glGetUniformLocation(billboard_shader, "flashlight_on"), player_flashlight_on);
    glUniform3fv(glGetUniformLocation(billboard_shader, "view_pos"), 1, glm::value_ptr(player_position));
    glm::vec3 normal = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.position"), 1, glm::value_ptr(player_position));
    glUniform3fv(glGetUniformLocation(billboard_shader, "player_flashlight.direction"), 1, glm::value_ptr(player_flashlight_direction));
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

    // prepare billboard shader for player gun
    glUniform2iv(glGetUniformLocation(billboard_shader, "extents"), 1, glm::value_ptr(resource_extents[resource_player_pistol]));
    glm::mat4 unit_mat4 = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "projection"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "view"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    glUniformMatrix4fv(glGetUniformLocation(billboard_shader, "model"), 1, GL_FALSE, glm::value_ptr(unit_mat4));
    normal = glm::normalize(glm::vec3(player_basis[2]));
    glUniform3fv(glGetUniformLocation(billboard_shader, "normal"), 1, glm::value_ptr(normal));
    glUniform1ui(glGetUniformLocation(billboard_shader, "frame"), player_animation.frame);

    // render player gun
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, player_animation.animation_info[player_animation.animation].texture_array);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Render crosshair
    glUseProgram(ui_shader);
    glUniform2iv(glGetUniformLocation(ui_shader, "extents"), 1, glm::value_ptr(crosshair_extents));
    glUniform3fv(glGetUniformLocation(ui_shader, "u_color"), 1, glm::value_ptr(crosshair_color));
    glBlendFunc(GL_ONE, GL_ZERO);
    glBindVertexArray(quad_vao);

    // top part
    glm::ivec2 crosshair_position = glm::ivec2(0, 8 + (int)(16.0f * player_recoil));
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
    std::string ammo_string = "AMMO: " + std::to_string(player_clip_ammo) + "/" + std::to_string(player_reserve_ammo);
    font_hack_10pt.render_text(ammo_string, SCREEN_WIDTH - (ammo_string.length() * 10), 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
}
