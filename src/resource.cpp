#include "resource.hpp"

#include "globals.hpp"

#include <glad/glad.h>
#include <stb_image.h>
#include <string>

const int TEXTURE_SIZE = 128;

unsigned int resource_textures;
unsigned int resource_player_pistol;

bool resource_load_all() {
    // load texture array
    glGenTextures(1, &resource_textures);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_textures);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE, NUM_TEXTURES, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    for (unsigned int i = 0; i < NUM_TEXTURES; i++) {
        int width, height, num_channels;
        const char * path = ("./res/texture/" + std::to_string(i) + ".png").c_str();
        unsigned char* data = stbi_load(path, &width, &height, &num_channels, 0);
        if (!data) {
            printf("Unable to load texture %s\n", path);
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, TEXTURE_SIZE, TEXTURE_SIZE, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glGenTextures(1, &resource_player_pistol);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, resource_player_pistol);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 5, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    for (unsigned int i = 0; i < 5; i++) {
        int width, height, num_channels;
        const char* path = ("./res/guns/pistol/" + std::to_string(i) + ".png").c_str();
        unsigned char* data = stbi_load(("./res/guns/pistol/" + std::to_string(i) + ".png").c_str(), &width, &height, &num_channels, 0);
        if (!data) {
            printf("Unable to load texture %s\n", path);
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, SCREEN_WIDTH, SCREEN_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return true;
}
