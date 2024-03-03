#pragma once

#include <vector>

#include "util/random.h"
#include "util/vector_math.h"

#include "types.h"
#include "vterm.h"

struct Map;
struct Ship;
struct UPlayer;
struct Universe;

enum class GameState
{
    Title,
    MainMenu,
    Ingame,
    GameOver,
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

struct Animation
{
    virtual ~Animation() {}
    virtual bool draw() = 0;
};

struct ProjectileAnimation : Animation
{
    vec2i from, to;
    u32 color;
    int character;

    std::vector<vec2i> points;

    ProjectileAnimation(const vec2i& from, const vec2i& to, u32 color, int character=0);

    bool draw() override;
};

char getProjectileCharacter(Direction dir);

struct Modal
{
    vec2i pos, size;
    sstring title;
    bool close = false;
    bool draw_border = true;

    Modal(vec2i pos, vec2i size, const sstring& title) : pos(pos), size(size), title(title) {}
    virtual ~Modal() {}

    virtual void draw() = 0;
};

struct Game
{
    int w=0, h=0;

    InfoLog log;
    Registry reg;
    pcg32 rng;
    TextBuffer* mapterm = nullptr;
    TextBuffer* uiterm = nullptr;

    Map* current_level = nullptr;
    Ship* player_ship = nullptr;
    Universe* universe = nullptr;
    UPlayer* uplayer = nullptr;

    bool show_universe = false;
    float transition = 0.0f;
    int last_universe_update = -1000;

    GameState state = GameState::Ingame;

    sstring console_input;
    int console_cursor = 0;
    bool console_input_displayed = false;

    Modal* modal = nullptr;

    std::vector<Animation*> animations;
    std::vector<Animation*> uanimations;
};
extern Game g_game;

void initGame(int w, int h);
void updateGame();

vec2i game_mouse_pos();
vec2f screen_mouse_pos();

bool drawButton(TextBuffer* term, vec2i pos, const char* label, u32 color);
