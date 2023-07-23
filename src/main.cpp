#ifdef _WIN32
    #define SDL_MAIN_HANDLED
#endif

#include "input.hpp"
#include "level.hpp"
#include "globals.hpp"
#include "shader.hpp"
#include "font.hpp"
#include "mesh.hpp"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <map>
#include <string>

SDL_Window* window;
SDL_GLContext context;

const unsigned long FRAME_TIME = 1000.0 / 60.0;
unsigned long last_time = SDL_GetTicks();
unsigned long last_second = SDL_GetTicks();
unsigned int frames = 0;
unsigned int fps = 0;

int main() {
    // Init engine
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_LoadLibrary(NULL);

    window = SDL_CreateWindow("learngl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        return -1;
    }

    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        printf("Error creating gl context: %s\n", SDL_GetError());
        return -1;
    }

    gladLoadGLLoader(SDL_GL_GetProcAddress);
    printf("Initialized OpenGL. Vendor %s\nRenderer %s\nVersion%s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

    if (glGenVertexArrays == NULL) {
        printf("glGenVertexArrays is null \n");
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);

    // Set OpenGL flags
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // Compile shaders
    unsigned int light_shader;
    bool success = shader_compile(&light_shader, "./shader/light_vertex.glsl", "./shader/light_fragment.glsl");
    if (!success) {
        return -1;
    }

    if(!font_init()) {
        return -1;
    }

    float light_vertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
    };

    unsigned int light_vao, light_vbo;
    glGenVertexArrays(1, &light_vao);
    glGenBuffers(1, &light_vbo);

    glBindVertexArray(light_vao);
    glBindBuffer(GL_ARRAY_BUFFER, light_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(light_vertices), light_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

    glBindVertexArray(0);

    float cube_vertices[] = {
        -7.0f, -3.0f, -10.0f,  0.0f,  0.0f, 1.0f,
         3.0f, -3.0f, -10.0f,  0.0f,  0.0f, 1.0f,
         3.0f,  3.0f, -10.0f,  0.0f,  0.0f, 1.0f,
         3.0f,  3.0f, -10.0f,  0.0f,  0.0f, 1.0f,
        -7.0f,  3.0f, -10.0f,  0.0f,  0.0f, 1.0f,
        -7.0f, -3.0f, -10.0f,  0.0f,  0.0f, 1.0f,

        -7.0f, -3.0f,  10.0f,  0.0f,  0.0f, -1.0f,
         3.0f, -3.0f,  10.0f,  0.0f,  0.0f, -1.0f,
         3.0f,  3.0f,  10.0f,  0.0f,  0.0f, -1.0f,
         3.0f,  3.0f,  10.0f,  0.0f,  0.0f, -1.0f,
        -7.0f,  3.0f,  10.0f,  0.0f,  0.0f, -1.0f,
        -7.0f, -3.0f,  10.0f,  0.0f,  0.0f, -1.0f,

        -7.0f,  3.0f,  10.0f, 1.0f,  0.0f,  0.0f,
        -7.0f,  3.0f, -10.0f, 1.0f,  0.0f,  0.0f,
        -7.0f, -3.0f, -10.0f, 1.0f,  0.0f,  0.0f,
        -7.0f, -3.0f, -10.0f, 1.0f,  0.0f,  0.0f,
        -7.0f, -3.0f,  10.0f, 1.0f,  0.0f,  0.0f,
        -7.0f,  3.0f,  10.0f, 1.0f,  0.0f,  0.0f,

         3.0f,  3.0f,  10.0f, -1.0f,  0.0f,  0.0f,
         3.0f,  3.0f, -10.0f, -1.0f,  0.0f,  0.0f,
         3.0f, -3.0f, -10.0f, -1.0f,  0.0f,  0.0f,
         3.0f, -3.0f, -10.0f, -1.0f,  0.0f,  0.0f,
         3.0f, -3.0f,  10.0f, -1.0f,  0.0f,  0.0f,
         3.0f,  3.0f,  10.0f, -1.0f,  0.0f,  0.0f,

        -7.0f, -3.0f, -10.0f,  0.0f, 1.0f,  0.0f,
         3.0f, -3.0f, -10.0f,  0.0f, 1.0f,  0.0f,
         3.0f, -3.0f,  10.0f,  0.0f, 1.0f,  0.0f,
         3.0f, -3.0f,  10.0f,  0.0f, 1.0f,  0.0f,
        -7.0f, -3.0f,  10.0f,  0.0f, 1.0f,  0.0f,
        -7.0f, -3.0f, -10.0f,  0.0f, 1.0f,  0.0f,

        -7.0f,  3.0f, -10.0f,  0.0f, -1.0f,  0.0f,
         3.0f,  3.0f, -10.0f,  0.0f, -1.0f,  0.0f,
         3.0f,  3.0f,  10.0f,  0.0f, -1.0f,  0.0f,
         3.0f,  3.0f,  10.0f,  0.0f, -1.0f,  0.0f,
        -7.0f,  3.0f,  10.0f,  0.0f, -1.0f,  0.0f,
        -7.0f,  3.0f, -10.0f,  0.0f, -1.0f,  0.0f
    };

    unsigned int cube_vao, cube_vbo;
    glGenVertexArrays(1, &cube_vao);
    glGenBuffers(1, &cube_vbo);

    glBindVertexArray(cube_vao);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 camera_move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
    float pitch = 0.0f;
    float yaw = -90.0f;

    input_set_mapping();

    Level level;
    if(!level.init()) {
        return -1;
    }

    // Game loop
    bool running = true;
    while (running) {
        // Timekeep
        unsigned long current_time = SDL_GetTicks();
        float delta = (float)(current_time - last_time) / 60.0f;
        last_time = current_time;

        if (current_time - last_second >= 1000) {
            fps = frames;
            frames = 0;
            last_second += 1000;
        }

        if (current_time - last_time < FRAME_TIME) {
            unsigned long delay_time = FRAME_TIME - (current_time - last_time);
            SDL_Delay(delay_time);
        }

        // Handle input
        input_prime_state();
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                } else {
                    running = false;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == 1 && SDL_GetRelativeMouseMode() == SDL_FALSE) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                input_handle_event(e);
            }
        }

        // Update
        level.update(delta);
        glm::vec3 camera_direction = glm::normalize(
                glm::vec3(
                    cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
                    sin(glm::radians(pitch)),
                    sin(glm::radians(yaw)) * cos(glm::radians(pitch))));

        camera_move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 forward_move_direction = glm::normalize(glm::vec3(camera_direction.x, 0.0f, camera_direction.z));
        /*
        if (keystate[SDL_SCANCODE_W]) {
            camera_move_direction = forward_move_direction;
        } else if (keystate[SDL_SCANCODE_S]) {
            camera_move_direction = -forward_move_direction;
        }
        if (keystate[SDL_SCANCODE_A]) {
            camera_move_direction = camera_move_direction - glm::normalize(glm::cross(forward_move_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        } else if (keystate[SDL_SCANCODE_D]) {
            camera_move_direction = camera_move_direction + glm::normalize(glm::cross(forward_move_direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        }
        if (glm::length(camera_move_direction) > 1) {
            camera_move_direction = glm::normalize(camera_move_direction);
        }
        camera_position = camera_position + (camera_move_direction * 0.1f);
        if (keystate[SDL_SCANCODE_Q]) {
            camera_position.y -= 0.1f;
        } else if (keystate[SDL_SCANCODE_E]) {
            camera_position.y += 0.1f;
        }
        */

        // Render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBlendFunc(GL_ONE, GL_ZERO);
        glUseProgram(level.texture_shader);

        glm::vec3 light_pos = glm::vec3(1.0f, 1.5f, -2.0f);
        glUniform3fv(glGetUniformLocation(level.texture_shader, "light_pos"), 1, glm::value_ptr(light_pos));
        glUniform3fv(glGetUniformLocation(level.texture_shader, "view_pos"), 1, glm::value_ptr(camera_position));

        // view / projection transformations
        glm::mat4 view;
        view = glm::lookAt(level.player.position, level.player.position - glm::vec3(level.player.basis[2]), glm::vec3(level.player.basis[1]));
        glm::mat4 projection;
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(level.texture_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(level.texture_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(level.texture_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        /*glBindVertexArray(cube_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);*/
        level.render();

        glm::mat4 light_model = glm::mat4(1.0f);
        light_model = glm::translate(light_model, light_pos);
        light_model = glm::scale(light_model, glm::vec3(0.2f));
        glUseProgram(light_shader);
        glUniformMatrix4fv(glGetUniformLocation(light_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(light_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(light_shader, "model"), 1, GL_FALSE, glm::value_ptr(light_model));
        glBindVertexArray(light_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // Render text
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        font_hack_10pt.render_text("FPS: " + std::to_string(fps), 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        SDL_GL_SwapWindow(window);
        frames++;
    }

    // Quit everything
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
