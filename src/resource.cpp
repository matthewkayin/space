#include "resource.hpp"

#include "globals.hpp"

#include <glad/glad.h>
#include <stb_image.h>
#include <string>

struct ResourceLoadInput {
    std::string path;
    unsigned int width;
    unsigned int height;
    unsigned int num_textures;
    unsigned int format;
    unsigned int wrap_s;
    unsigned int wrap_t;
    unsigned int min_filter;
    unsigned int mag_filter;
    bool generate_mipmaps;
};

const int TEXTURE_SIZE = 128;

unsigned int resource_textures;
unsigned int resource_player_pistol;
unsigned int resource_bullet_hole;
std::map<unsigned int, glm::ivec2> resource_extents;

bool success = true;

void resource_load(unsigned int* id, ResourceLoadInput input) {
    glGenTextures(1, id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, *id);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, input.format, input.width, input.height, input.num_textures, 0, input.format, GL_UNSIGNED_BYTE, NULL);
    for (unsigned int i = 0; i < input.num_textures; i++) {
        int width, height, num_channels;
        std::string path = (input.path + "/" + std::to_string(i) + ".png");
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
        if (!data) {
            printf("Unable to load texture %s\n", path.c_str());
            success = false;
            return;
        }
        if (width != input.width || height != input.height) {
            printf("width and height don't match texture %s\n", path.c_str());
            success = false;
            return;
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, input.width, input.height, 1, input.format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    if (input.generate_mipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, input.wrap_s);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, input.wrap_t);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, input.min_filter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, input.mag_filter);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    resource_extents.insert(std::pair<unsigned int, glm::ivec2>(*id, glm::ivec2(input.width / 2, input.height / 2)));
}

bool resource_load_all() {
    resource_load(&resource_textures, {
        .path = "./res/texture",
        .width = TEXTURE_SIZE,
        .height = TEXTURE_SIZE,
        .num_textures = NUM_TEXTURES,
        .format = GL_RGB,
        .wrap_s = GL_REPEAT,
        .wrap_t = GL_REPEAT,
        .min_filter = GL_NEAREST_MIPMAP_LINEAR,
        .mag_filter = GL_NEAREST_MIPMAP_LINEAR,
        .generate_mipmaps = true
    });
    resource_load(&resource_player_pistol, {
        .path = "./res/guns/pistol",
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .num_textures = 31,
        .format = GL_RGBA,
        .wrap_s = GL_CLAMP_TO_EDGE,
        .wrap_t = GL_CLAMP_TO_EDGE,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
        .generate_mipmaps = false
    });
    resource_load(&resource_bullet_hole, {
        .path = "./res/bullet_hole",
        .width = 32,
        .height = 32,
        .num_textures = 1,
        .format = GL_RGBA,
        .wrap_s = GL_CLAMP_TO_EDGE,
        .wrap_t = GL_CLAMP_TO_EDGE,
        .min_filter = GL_NEAREST_MIPMAP_LINEAR,
        .mag_filter = GL_NEAREST_MIPMAP_LINEAR,
        .generate_mipmaps = true
    });

    return success;
}
