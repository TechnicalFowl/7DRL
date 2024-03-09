#pragma once

#include <vector>

#include "util/vector_math.h"
#include "util/string.h"

struct TextBuffer;

struct Window
{
    int width, height;

    u32 shader;
    
    Texture font_texture;

    u64 frame_count = 0;

    float map_zoom = 1.0f;
};
extern Window g_window;

void window_open(const char* title, int w, int h);

void render_buffer(TextBuffer* term, float zoom);

const char* GetKeyName(int key);
