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

    Image img = LoadImage("assets/rl_text16.png");
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

const char* GetKeyName(int key)
{
    switch (key)
    {
    case 0: return "UNBOUND";
    case KEY_APOSTROPHE: return "'";
    case KEY_COMMA: return ",";
    case KEY_MINUS: return "-";
    case KEY_PERIOD: return ".";
    case KEY_SLASH: return "/";
    case KEY_ZERO: return "0";
    case KEY_ONE: return "1";
    case KEY_TWO: return "2";
    case KEY_THREE: return "3";
    case KEY_FOUR: return "4";
    case KEY_FIVE: return "5";
    case KEY_SIX: return "6";
    case KEY_SEVEN: return "7";
    case KEY_EIGHT: return "8";
    case KEY_NINE: return "9";
    case KEY_SEMICOLON: return ";";
    case KEY_EQUAL: return "=";
    case KEY_A: return "A";
    case KEY_B: return "B";
    case KEY_C: return "C";
    case KEY_D: return "D";
    case KEY_E: return "E";
    case KEY_F: return "F";
    case KEY_G: return "G";
    case KEY_H: return "H";
    case KEY_I: return "I";
    case KEY_J: return "J";
    case KEY_K: return "K";
    case KEY_L: return "L";
    case KEY_M: return "M";
    case KEY_N: return "N";
    case KEY_O: return "O";
    case KEY_P: return "P";
    case KEY_Q: return "Q";
    case KEY_R: return "R";
    case KEY_S: return "S";
    case KEY_T: return "T";
    case KEY_U: return "U";
    case KEY_V: return "V";
    case KEY_W: return "W";
    case KEY_X: return "X";
    case KEY_Y: return "Y";
    case KEY_Z: return "Z";
    case KEY_LEFT_BRACKET: return "[";
    case KEY_BACKSLASH: return "\\";
    case KEY_RIGHT_BRACKET: return "]";
    case KEY_GRAVE: return "`";
    case KEY_SPACE: return "Space";
    case KEY_ESCAPE: return "Esc";
    case KEY_ENTER: return "Enter";
    case KEY_TAB: return "Tab";
    case KEY_BACKSPACE: return "Backspace";
    case KEY_INSERT: return "Ins";
    case KEY_DELETE: return "Del";
    case KEY_RIGHT: return "right";
    case KEY_LEFT: return "left";
    case KEY_DOWN: return "down";
    case KEY_UP: return "up";
    case KEY_PAGE_UP: return "Page up";
    case KEY_PAGE_DOWN: return "Page down";
    case KEY_HOME: return "Home";
    case KEY_END: return "End";
    case KEY_CAPS_LOCK: return "Caps lock";
    case KEY_SCROLL_LOCK: return "Scroll down";
    case KEY_NUM_LOCK: return "Num lock";
    case KEY_PRINT_SCREEN: return "Print screen";
    case KEY_PAUSE: return "Pause";
    case KEY_F1: return "F1";
    case KEY_F2: return "F2";
    case KEY_F3: return "F3";
    case KEY_F4: return "F4";
    case KEY_F5: return "F5";
    case KEY_F6: return "F6";
    case KEY_F7: return "F7";
    case KEY_F8: return "F8";
    case KEY_F9: return "F9";
    case KEY_F10: return "F10";
    case KEY_F11: return "F11";
    case KEY_F12: return "F12";
    case KEY_LEFT_SHIFT: return "Shift left";
    case KEY_LEFT_CONTROL: return "Control left";
    case KEY_LEFT_ALT: return "Alt left";
    case KEY_LEFT_SUPER: return "Super left";
    case KEY_RIGHT_SHIFT: return "Shift right";
    case KEY_RIGHT_CONTROL: return "Control right";
    case KEY_RIGHT_ALT: return "Alt right";
    case KEY_RIGHT_SUPER: return "Super right";
    case KEY_KB_MENU: return "KB menu";
    case KEY_KP_0: return "Keypad 0";
    case KEY_KP_1: return "Keypad 1";
    case KEY_KP_2: return "Keypad 2";
    case KEY_KP_3: return "Keypad 3";
    case KEY_KP_4: return "Keypad 4";
    case KEY_KP_5: return "Keypad 5";
    case KEY_KP_6: return "Keypad 6";
    case KEY_KP_7: return "Keypad 7";
    case KEY_KP_8: return "Keypad 8";
    case KEY_KP_9: return "Keypad 9";
    case KEY_KP_DECIMAL: return "Keypad .";
    case KEY_KP_DIVIDE: return "Keypad /";
    case KEY_KP_MULTIPLY: return "Keypad *";
    case KEY_KP_SUBTRACT: return "Keypad -";
    case KEY_KP_ADD: return "Keypad +";
    case KEY_KP_ENTER: return "Keypad Enter";
    case KEY_KP_EQUAL: return "Keypad =";
    default: break;
    }

    static char temp[16];
    snprintf(temp, 15, "#%d", key);
    return temp;
}
