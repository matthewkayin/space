#include "texture.hpp"

#include <glad/glad.h>
#include <stb_image.h>

unsigned int texture_load(const char* path) {
    unsigned int texture_id;
    glGenTextures(1, &texture_id);

    int width, height, num_components;
    unsigned char* data = stbi_load(path, &width, &height, &num_components, 0);
    if (data) {
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        printf("Texture failed to load at path %s\n", path);
    }
    stbi_image_free(data);

    return texture_id;
}
