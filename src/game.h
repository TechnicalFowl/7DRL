#pragma once

#include <vector>

#include "util/random.h"
#include "util/vector_math.h"

#include "types.h"
#include "vterm.h"

struct Map;
struct Ship;
struct UShip;
struct UPlayer;
struct Universe;

enum class GameState
{
    MainMenu,
    Settings,
    Ingame,
    PauseMenu,
    GameOver,
    Victory,
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
    int tick_created;

    Animation();
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

struct RailgunAnimation : Animation
{
    vec2i from, to;
    u32 color;
    int character;

    vec2i last;

    std::vector<vec2i> points;
    std::vector<vec2i> hits;
    std::vector<vec2i> misses;

    RailgunAnimation(u32 color, int c);

    bool draw() override;
};

struct ExplosionAnimation : Animation
{
    vec2i center;
    int radius;

    float step = 0.0f;

    ExplosionAnimation(vec2i c, int r);

    bool draw() override;
};

struct ShipMoveAnimation : Animation
{
    UShip* ship;
    vec2i from, to;
    int character;
    u32 color;

    std::vector<vec2i> points;

    ShipMoveAnimation(UShip* s, vec2i f, vec2i t);
    virtual ~ShipMoveAnimation();

    bool draw() override;
};

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

struct HailModal : Modal
{
    UShip* ship;
    bool was_masked = false;

    HailModal(UShip* s);

    void draw() override;
};

struct Game
{
    int w=0, h=0;

    InfoLog log;
    Registry reg;
    pcg32 rng;
    TextBuffer* mapterm = nullptr;
    TextBuffer* uiterm = nullptr;

    int credits = 1000;
    int scrap = 15;

    std::vector<Ship*> ships;
    Map* current_level = nullptr;
    Ship* player_ship = nullptr;
    Universe* universe = nullptr;
    UPlayer* uplayer = nullptr;

    bool show_universe = false;
    float transition = 0.0f;
    int last_universe_update = -1000;

    GameState state = GameState::MainMenu;
    sstring gameover_reason;

    Modal* modal = nullptr;
    bool modal_close = false;

    int music_volume = 8;
    int sound_volume = 8;

    int key_left = KEY_LEFT;
    int key_right = KEY_RIGHT;
    int key_up = KEY_UP;
    int key_down = KEY_DOWN;
    int key_fire = KEY_F;
    int key_railgun = KEY_Z;
    int key_dock = KEY_D;
    int key_open = KEY_O;
    int key_use = KEY_U;
    int key_pickup = KEY_COMMA;
    int key_wait = KEY_SPACE;
    int key_hail = KEY_H;

    bool is_aiming_hail = false;

    std::vector<Animation*> animations;
    std::vector<Animation*> uanimations;
};
extern Game g_game;

void initGame(int w, int h);
void startGame();
void updateGame();

vec2i game_mouse_pos();
vec2f screen_mouse_pos();

bool drawButton(TextBuffer* term, vec2i pos, const char* label, u32 color, bool disabled=false);
