#include "game.h"

#include <cstdarg>

#include "actor.h"
#include "map.h"
#include "vterm.h"
#include "window.h"

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

    {
        Registry& reg = g_game.reg;
        reg.terrain_info[int(Terrain::Empty)] = TerrainInfo(Terrain::Empty, "Empty", TileEmpty, 0, 0xFF000000, true);
        reg.terrain_info[int(Terrain::StoneWall)] = TerrainInfo(Terrain::StoneWall, "Stone Wall", TileFull, 0xFFB0B0B0, 0, false);
        reg.terrain_info[int(Terrain::DirtFloor)] = TerrainInfo(Terrain::DirtFloor, "Dirt Floor", TileFull, 0xFF2F1b08, 0, true);

        reg.actor_info[int(ActorType::Player)] = ActorInfo(ActorType::Player, "Player", '@', 0xFFFFFFFF, LayerPriority_Actors + 10, false, 10);
        reg.actor_info[int(ActorType::Goblin)] = ActorInfo(ActorType::Goblin, "Goblin", 'g', 0xFF00FF00, LayerPriority_Actors + 1, false, 5);
        reg.actor_info[int(ActorType::GroundItem)] = ActorInfo(ActorType::GroundItem, "Item", '?', 0xFFFF00FF, LayerPriority_Objects + 1, true, 999);
        reg.actor_info[int(ActorType::Door)] = ActorInfo(ActorType::Door, "Door", '=', 0xFFC0C0C0, LayerPriority_Objects, false, 50);

        reg.item_type_info[int(ItemType::Generic)] = ItemTypeInfo(ItemType::Generic, "Generic", '?', 0xFFFFFFFF);
    }

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
    Map& map = *g_game.current_level;

    bool do_turn = false;
    if (input_key_pressed(GLFW_KEY_UP))
    {
        do_turn = true;
        map.player->tryMove(map, vec2i(0, 1));
    }
    if (input_key_pressed(GLFW_KEY_DOWN))
    {
        do_turn = true;
        map.player->tryMove(map, vec2i(0, -1));
    }
    if (input_key_pressed(GLFW_KEY_RIGHT))
    {
        do_turn = true;
        map.player->tryMove(map, vec2i(1, 0));
    }
    if (input_key_pressed(GLFW_KEY_LEFT))
    {
        do_turn = true;
        map.player->tryMove(map, vec2i(-1, 0));
    }
    if (input_key_pressed(GLFW_KEY_O))
    {
        do_turn = true;
        map.player->next_action = ActionData(Action::Open, map.player);
    }

    if (do_turn)
    {
        std::vector<ActionData> actions;
        for (Actor* a : map.actors)
        {
            ActionData act = a->update(map, g_game.rng);
            if (act.action != Action::Wait) actions.emplace_back(act);
        }

        for (ActionData& act : actions)
        {
            act.apply(map);
        }
        map.turn++;
    }

    g_game.term->clear();
    g_game.term->write(vec2i(10, 10), "Hello, world!", 0xffffffff, 0);

    map.render(*g_game.term, map.player->pos);
}
