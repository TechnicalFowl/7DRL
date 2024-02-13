#pragma once

#include <vector>

#include "util/random.h"
#include "util/vector_math.h"

#include "types.h"
#include "vterm.h"

struct Map;

enum class SidebarUI
{
    Character,
    GameLog,
    Examine,
};

struct InfoLog
{
    struct Entry
    {
        sstring msg;
        u32 color = 0xFFFFFFFF;
        int count = 1;

        Entry(const sstring& msg) : msg(msg) {}
        Entry(const sstring& msg, u32 c) : msg(msg), color(c) {}

        sstring getMessage() const
        {
            sstring s = msg;
            if (count > 1)
                s.appendf(" (x%d)", count);
            return s;
        }
    };

    std::vector<Entry> entries;

    void log(const sstring& msg);
    void logf(const char* fmt, ...);
    void log(u32 color, const sstring& msg);
    void logf(u32 color, const char* fmt, ...);
};

struct Game
{
    int w, h;

    InfoLog log;
    Registry reg;
    pcg32 rng;
    TextBuffer* term;

    Map* current_level;

    SidebarUI sidebar = SidebarUI::Character;
};
extern Game g_game;

void initGame(int w, int h);
void updateGame();

vec2i game_mouse_pos();
vec2f screen_mouse_pos();
