#include "globals.hpp"

#include <fstream>
#include <cstdio>

bool disable_noise = false;

bool config_init() {
    std::ifstream file("./config.ini");
    if (!file.is_open()) {
        printf("Could not open config.ini\n");
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        unsigned int first_char_index = 0;
        for (unsigned int i = 0; i < line.length(); i++) {
            if (line[i] != ' ') {
                first_char_index = i;
                break;
            }
        }
        if (line[first_char_index] == '#' || line[first_char_index] == '[') {
            continue;
        }

        size_t equals_index = line.find("=");
        std::string key = line.substr(first_char_index, equals_index);
        std::string value = line.substr(equals_index + 1);

        if (key == "window_width") {
            WINDOW_WIDTH = std::stoul(value);
        } else if (key == "window_height") {
            WINDOW_HEIGHT = std::stoul(value);
        } else if (key == "disable_noise") {
            disable_noise = value == "1";
        }
    }

    return true;
}
