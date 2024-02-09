#include "game.h"

#include <cstdarg>

#include "actor.h"
#include "map.h"
#include "vterm.h"

Game g_game;

void InfoLog::log(const sstring& msg)
{
    entries.emplace_back(msg);
}

void InfoLog::logf(const char* fmt, ...)
{
    sstring msg;
    va_list args;
    va_start(args, fmt);
    msg.vappend(fmt, args);
    va_end(args);
    entries.emplace_back(msg);
}

void InfoLog::log(u32 color, const sstring& msg)
{
    entries.emplace_back(msg, color);
}

void InfoLog::logf(u32 color, const char* fmt, ...)
{
    sstring msg;
    va_list args;
    va_start(args, fmt);
    msg.vappend(fmt, args);
    va_end(args);
    entries.emplace_back(msg, color);
}

void initGame(int w, int h)
{
    g_game.term = new TextBuffer(w, h);

    g_game.log.log("Welcome.");

    g_game.current_level = new Map("level_0");
    Map& map = *g_game.current_level;

    Player* player = new Player(vec2i(0, 0));
    map.player = player;
    map.spawn(player);

    Monster* goblin = new Monster(map.findNearestEmpty(vec2i(8, 8), Terrain::DirtFloor), ActorType::Goblin);
    map.spawn(goblin);
}

void updateGame()
{

    g_game.term->clear();
    g_game.term->write(vec2i(10, 10), "Hello, world!", 0xffffffff, 0);
}
