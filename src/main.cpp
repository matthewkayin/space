#include "shader.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main() {
    // Init everything
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_LoadLibrary(NULL);

    SDL_Window* window = SDL_CreateWindow("learngl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
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

    unsigned long last_time = SDL_GetTicks();

    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    unsigned int shader = shader_compile("./shader/vertex.glsl", "./shader/fragment.glsl");
    unsigned int light_shader = shader_compile("./shader/light_vertex.glsl", "./shader/fragment.glsl");

    int width, height, nr_channels;
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char* data = stbi_load("./container.jpg", &width, &height, &nr_channels, 0);
    if (!data) {
        printf("Failed to load texture\n");
        return -1;
    }

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, -0.5f,

        -0.5f, -0.5f, 0.5f,
        0.5f, -0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f
    };

    unsigned int indices[] = {
        0, 1, 3,
        3, 1, 2,

        1, 5, 2,
        2, 5, 6,

        5, 4, 6,
        6, 4, 7,

        4, 0, 7,
        7, 0, 3,

        3, 2, 7,
        7, 2, 6,

        4, 5, 0,
        0, 5, 1
    };

    float ship_vertices[] = {
        0.0f, 0.2f, -0.1f,
        -0.4f, 0.0f, -0.6f,
        0.4f, 0.0f, -0.6f,

        0.0f, 0.3f, 0.6f,
        -0.8f, 0.2f, 0.6f,
        0.8f, 0.2f, 0.6f,

        -1.0f, 0.0f, 0.4f,
        1.0f, 0.0f, 0.4f,

        -1.0f, 0.0f, 0.6f,
        1.0f, 0.0f, 0.6f,

        -0.4f, -0.2f, 0.6f,
        0.4f, -0.2f, 0.6f
    };
    unsigned int ship_indices[] = {
        // top
        0, 1, 2,

        0, 5, 3,
        0, 3, 4,

        0, 4, 1,
        0, 2, 5,

        4, 6, 1,
        5, 2, 7,

        4, 8, 6,
        5, 7, 9,

        // bottom
        1, 8, 10,
        9, 2, 11,

        1, 6, 8,
        9, 7, 2,

        2, 1, 10,
        2, 10, 11,

        // back
        3, 11, 10,
        4, 3, 10,
        3, 5, 11,
        5, 9, 11,
        4, 10, 8
    };

    Mesh cube = Mesh(vertices, sizeof(vertices), indices, sizeof(indices));
     Mesh ship = Mesh(ship_vertices, sizeof(ship_vertices), ship_indices, sizeof(ship_indices));

    glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 camera_move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
    float pitch = 0.0f;
    float yaw = -90.0f;

    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    glm::vec3 model_rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    bool is_wireframe = true;

    // Game loop
    bool running = true;
    while (running) {
        unsigned long current_time = SDL_GetTicks();
        float delta = (float)(current_time - last_time) / 60.0f;
        last_time = current_time;

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
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_w) {
                if (is_wireframe) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                } else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                }
                is_wireframe = !is_wireframe;
            } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == 1 && SDL_GetRelativeMouseMode() == SDL_FALSE) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else if (SDL_GetRelativeMouseMode() == SDL_TRUE && e.type == SDL_MOUSEMOTION) {
                model_rotation.x += e.motion.xrel * 0.2f;
                model_rotation.y += e.motion.yrel * 0.2f;
                /*
                yaw += e.motion.xrel * 0.1f;
                pitch -= e.motion.yrel * 0.1f;
                if (pitch > 89.0f) {
                    pitch = 89.0f;
                } else if (pitch < -89.0f) {
                    pitch = -89.0f;
                }
                */
            } else if (e.type == SDL_MOUSEWHEEL) {
                camera_position.z += 0.1f * e.wheel.y;
            }
        }

        glm::vec3 camera_direction = glm::normalize(
                glm::vec3(
                    cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
                    sin(glm::radians(pitch)),
                    sin(glm::radians(yaw)) * cos(glm::radians(pitch))));

        camera_move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
        // glm::vec3 forward_move_direction = glm::normalize(glm::vec3(camera_direction.x, 0.0f, camera_direction.z));
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
        */
        if (glm::length(camera_move_direction) > 1) {
            camera_move_direction = glm::normalize(camera_move_direction);
        }
        camera_position = camera_position + (camera_move_direction * 0.1f);

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::vec3 light_pos = glm::vec3(2.0f, 3.0f, -1.0f);
        glUniform3fv(glGetUniformLocation(shader, "light_pos"), 1, glm::value_ptr(light_pos));
        glUniform3fv(glGetUniformLocation(shader, "view_pos"), 1, glm::value_ptr(camera_position));

        // view / projection transformations
        glm::mat4 view;
        view = glm::lookAt(camera_position, camera_position + camera_direction, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection;
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f));
        model = glm::rotate(model, glm::radians(model_rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(model_rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        cube.draw(shader);
        // cube.draw(shader);
        //
        glUseProgram(light_shader);

        glm::mat4 light_model = glm::mat4(1.0f);
        light_model = glm::scale(light_model, glm::vec3(0.3f));
        light_model = glm::translate(light_model, light_pos);
        glUniformMatrix4fv(glGetUniformLocation(light_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(light_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(light_shader, "model"), 1, GL_FALSE, glm::value_ptr(light_model));
        cube.draw(light_shader);

        SDL_GL_SwapWindow(window);
    }

    // Quit everything
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
