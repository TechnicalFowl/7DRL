#include "window.h"

#include <cstdio>

#include "vterm.h"

Window g_window;

void window_open(const char* title, int w, int h)
{
    InitWindow(w, h, title);
    SetTargetFPS(60);
    SetWindowMinSize(1280, 720);
    SetWindowMaxSize(2560, 1440);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    g_window.width = w;
    g_window.height = h;

    Image img = LoadImage("rl_text16.png");
    g_window.font_texture = LoadTextureFromImage(img);
    UnloadImage(img);
}

struct CharTexture
{
    char c;
    Rectangle uv;
};

Rectangle makeCharTexture(int x, int y)
{
    float x0 = x * 8.0f;
    float y0 = y * 16.0f;
    return Rectangle{ x0 + 0.01f, y0 + 0.01f, 7.98f, 15.98f };
}

Rectangle getCharTexture(char c)
{
    static Rectangle char_map[256]{ 0 };
    static bool char_map_initialized = false;
    if (!char_map_initialized)
    {
        char_map[' '] = makeCharTexture(31, 0);
        for (int i = 0; i < 26; ++i)
        {
            char_map['A' + i] = makeCharTexture(i, 0);
            char_map['a' + i] = makeCharTexture(i, 1);
        }
        for (int i = 0; i < 5; ++i)
        {
            char_map['0' + i] = makeCharTexture(i + 26, 0);
            char_map['0' + i + 5] = makeCharTexture(i + 26, 1);
        }
        for (int i = 0; i < 32; ++i)
        {
            char c = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"[i];
            char_map[c] = makeCharTexture(i, 2);
        }
        // SpecialChars
        for (int i = 1; i < MaxSpecialChars; ++i)
        {
            char_map[i] = makeCharTexture(i - 1, 3);
        }
        char_map_initialized = true;
    }
    return char_map[c];
}

Rectangle getTexture(int id)
{
    vec2i size(g_window.font_texture.width, g_window.font_texture.height);
    vec2i grid(16, 16);

    int tw = size.x / grid.x;
    int th = size.y / grid.y;

    int tx = id % tw;
    int ty = id / tw;
    return Rectangle{ tx * 16.0f + 0.01f, ty * 16.0f + 0.01f, 15.98f, 15.98f };
}

Color makeColor(u32 c)
{
    Color col = { 0 };
    col.a = (c >> 24);
    col.r = (c >> 16) & 0xFF;
    col.g = (c >> 8) & 0xFF;
    col.b = (c) & 0xFF;
    return col;
}

void render_buffer(TextBuffer* term, float zoom)
{
    Camera2D camera = { 0 };
    camera.target = Vector2{ term->w / 2.0f, term->h / 2.0f };
    camera.offset = Vector2{ g_window.width / 2.0f, g_window.height / 2.0f };
    camera.zoom = zoom * 16.0f;

    BeginMode2D(camera);

    #define invy(y) term->invert ? term->h - y : y

    for (int y = 0; y < term->h; ++y)
    {
        for (int x = 0; x < term->w; ++x)
        {
            TextBuffer::Char& ch = term->buffer[x + y * term->w];
            if ((ch.bg & 0xFFFFFF) != 0)
            {
                DrawRectangle(x, invy(y), 1, 1, makeColor(ch.bg));
            }
            if (ch.text[0] > TileEmpty)
            {
                Rectangle source = getTexture(ch.text[0] - 64);
                Rectangle dest{ float(x), float(invy(y)), 1.0f, 1.0f };
                DrawTexturePro(g_window.font_texture, source, dest, Vector2(), 0.0f, makeColor(ch.color[0]));
            }
            else if (ch.text[1] == 0xFFFF)
            {
                Rectangle source = getCharTexture(ch.text[0]);
                Rectangle dest{ x + 0.25f, float(invy(y)), 0.5f, 1.0f };
                DrawTexturePro(g_window.font_texture, source, dest, Vector2(), 0.0f, makeColor(ch.color[0]));
            }
            else
            {
                for (int j = 0; j < 2; ++j)
                {
                    if (ch.text[j] != TileEmpty)
                    {
                        Rectangle source = getCharTexture(ch.text[j]);
                        float x0 = j == 0 ? (float)x : (float)x + 0.5f;
                        Rectangle dest{ x0, float(invy(y)), 0.5f, 1.0f };
                        DrawTexturePro(g_window.font_texture, source, dest, Vector2(), 0.0f, makeColor(ch.color[j]));
                    }
                }
            }
            if ((ch.overlay & 0xFFFFFF) != 0)
            {
                DrawRectangle(x, invy(y), 1, 1, makeColor(ch.overlay));
            }
        }
    }

    EndMode2D();
}
