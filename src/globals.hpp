#pragma once

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 360;

const int NUM_TEXTURES = 3;

extern bool edit_mode;
extern unsigned int quad_vao;
extern float elapsed;
extern float screen_anim_timer;

// config
extern unsigned int WINDOW_WIDTH;
extern unsigned int WINDOW_HEIGHT;
extern bool disable_noise;

bool config_init();
