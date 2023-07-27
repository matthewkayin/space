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

    window = SDL_CreateWindow("zerog", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
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

        // Render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBlendFunc(GL_ONE, GL_ZERO);

        level.render();

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
