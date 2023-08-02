#ifdef _WIN32
    #define SDL_MAIN_HANDLED
#endif

#include "edit.hpp"
#include "input.hpp"
#include "level.hpp"
#include "globals.hpp"
#include "shader.hpp"
#include "font.hpp"
#include "scene.hpp"
#include "resource.hpp"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <map>
#include <string>

bool edit_mode;
unsigned int quad_vao;

SDL_Window* window;
SDL_GLContext context;

unsigned int WINDOW_WIDTH = 1280;
unsigned int WINDOW_HEIGHT = 720;

const unsigned long FRAME_TIME = 1000.0 / 60.0;
unsigned long last_time = SDL_GetTicks();
unsigned long last_second = SDL_GetTicks();
unsigned int frames = 0;
unsigned int fps = 0;

int main(int argc, char** argv) {
    edit_mode = argc > 1 && std::string(argv[1]) == "--edit";

    // Init engine
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_LoadLibrary(NULL);

    glm::ivec2 window_position = glm::ivec2(SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED);
    if (edit_mode) {
        SDL_Rect display_bounds;
        SDL_GetDisplayBounds(0, &display_bounds);
        window_position.x = (display_bounds.w / 2) + (SCREEN_WIDTH / 4);
        window_position.y = (display_bounds.h / 2) - (SCREEN_HEIGHT / 2);
    }

    window = SDL_CreateWindow("zerog", window_position.x, window_position.y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
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

    if (!shader_compile_all()) {
        return -1;
    }
    if (!resource_load_all()) {
        return -1;
    }

    font_init();
    input_set_mapping();
    level_init();
    scene_init();

    glUseProgram(screen_shader);
    glUniform1i(glGetUniformLocation(screen_shader, "screen_texture"), 0);

    // setup edit mode
    unsigned int main_window_id = SDL_GetWindowID(window);
    if (edit_mode) {
        if (!edit_init()) {
            return false;
        }
    }

    unsigned int quad_vbo;

    float quad_vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);

    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    unsigned int texture_color_buffer;
    glGenTextures(1, &texture_color_buffer);
    glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_color_buffer, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH, SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer not complete!\n");
        return -1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Game loop
    bool running = true;
    while (running) {
        // Timekeep
        unsigned long current_time = SDL_GetTicks();
        if (current_time - last_time < FRAME_TIME) {
            continue;
        }

        float delta = (float)(current_time - last_time) / 60.0f;
        last_time = current_time;

        if (current_time - last_second >= 1000) {
            fps = frames;
            frames = 0;
            last_second += 1000;
        }

        // Handle input
        input_prime_state();
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT || (edit_mode && e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                } else {
                    running = false;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && SDL_GetRelativeMouseMode() == SDL_FALSE && (!edit_mode || e.button.windowID == main_window_id)) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else if (SDL_GetRelativeMouseMode() == SDL_TRUE || edit_mode) {
                input_handle_event(e);
            }
        }

        // Update
        if (!edit_mode) {
            scene_update(delta);
        } else {
            if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                level_edit_update(delta);
            } else {
                edit_update();
            }
        }

        // Render onto framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glBlendFunc(GL_ONE, GL_ZERO);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // edit.render();
        scene_render();

        // Render text
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        font_hack_10pt.render_text("FPS: " + std::to_string(fps), 0.0f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        // Render edit window
        if (edit_mode) {
            edit_render();
        }

        // Render framebuffer to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glBlendFunc(GL_ONE, GL_ZERO);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(screen_shader);
        glBindVertexArray(quad_vao);
        glBindTexture(GL_TEXTURE_2D, texture_color_buffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        SDL_GL_SwapWindow(window);
        frames++;
    }

    // Quit everything
    if (edit_mode) {
        edit_quit();
    }
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
