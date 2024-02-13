#include "game.h"

#include <cstdarg>

#include "actor.h"
#include "map.h"
#include "procgen.h"
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

std::vector<sstring> smartSplit(const sstring& s, int row_size)
{
    int last = 0;
    std::vector<sstring> rows;
    while (last < (int)s.size() && (int)s.size() - last > row_size)
    {
        int prev = s.lastIndexOf(' ', last + row_size);
        if (prev == -1)
            prev = last + row_size;
        rows.emplace_back(s.substring(last, prev));
        last = prev + 1;
    }
    if (last < (int)s.size())
        rows.emplace_back(s.substring(last));
    return rows;
}

void initGame(int w, int h)
{
    g_game.w = w;
    g_game.h = h;
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
        reg.item_type_info[int(ItemType::Equipment)] = ItemTypeInfo(ItemType::Equipment, "Equipment", ')', 0xFFFFFFFF);
    }

    g_game.current_level = new Map("level_0");
    Map& map = *g_game.current_level;

    generate(map);

    Player* player = new Player(vec2i(0, 0));
    player->inate_modifiers.emplace_back(ModifierType::Accuracy, 100.0f, 1.0f);
    player->inate_modifiers.emplace_back(ModifierType::Damage, DamageType::Blunt, 1.0f, 1.0f);
    map.player = player;
    map.spawn(player);

    Monster* goblin = new Monster(map.findNearestEmpty(vec2i(8, 8), Terrain::DirtFloor), ActorType::Goblin);
    goblin->inate_modifiers.emplace_back(ModifierType::Accuracy, 50.0f, 1.0f);
    Equipment* goblin_spear = goblin->equipment[int(EquipmentSlot::MainHand)] = new Equipment('/', 0xffffffff, ItemType::Equipment, "Goblin Spear", EquipmentSlot::MainHand);
    goblin_spear->modifiers.emplace_back(ModifierType::Damage, DamageType::Piercing, 3.0f, 1.0f);
    map.spawn(goblin);
}

vec2i game_mouse_pos()
{
    vec2i bl = g_game.current_level->player->pos - vec2i(25, 22);
    vec2f mouse_posf = g_window.inputs.mouse_pos / 16;
    return vec2i(scalar::floori(mouse_posf.x), scalar::floori(g_game.term->h - mouse_posf.y)) + bl;
}

vec2f screen_mouse_pos()
{
    vec2f mouse_posf = g_window.inputs.mouse_pos / 16;
    return vec2f(mouse_posf.x, g_game.term->h - mouse_posf.y);
}

void drawUIFrame(TextBuffer* term, vec2i min, vec2i max, const char* title)
{
    term->fillBg(vec2i(min.x, min.y), vec2i(max.x, min.y), 0xFF404040, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x, min.y), vec2i(min.x, max.y), 0xFF404040, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x, max.y), vec2i(max.x, max.y), 0xFF404040, LayerPriority_UI - 2);
    term->fillBg(vec2i(max.x, min.y), vec2i(max.x, max.y), 0xFF404040, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x + 1, min.y + 1), vec2i(max.x - 1, max.y - 1), 0xFF000000, LayerPriority_UI - 2);
    size_t title_len = strlen(title);
    term->fillText(vec2i(min.x * 2, max.y), vec2i(min.x * 2 + 6, max.y), '=', 0xFFA0A0A0, LayerPriority_UI - 1);
    term->fillText(vec2i(min.x * 2 + 9 + title_len, max.y), vec2i(max.x*2 + 1, max.y), '=', 0xFFA0A0A0, LayerPriority_UI - 1);
    term->write(vec2i(min.x * 2 + 8, max.y), title, 0xFFA0A0A0, LayerPriority_UI - 1);
}

bool drawButton(TextBuffer* term, vec2i pos, const char* label, u32 color)
{
    sstring text; text.appendf("[%s]", label);

    vec2f mouse = screen_mouse_pos();
    bool hovered = mouse.x >= pos.x && mouse.x < pos.x + (text.size() / 2.0f) && scalar::floori(mouse.y) == pos.y;

    term->write(vec2i(pos.x * 2, pos.y), text.c_str(), hovered ? 0xFF00FF00 : color, LayerPriority_UI);
    
    return hovered && input_mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT);
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

    vec2i mouse_pos = game_mouse_pos();

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
            act.apply(map, g_game.rng);
        }
        map.turn++;
    }

    g_game.term->clear();

#if 0
    // Pathfinding debug
    auto path = map.findPath(map.player->pos, mouse_pos);
    g_game.term->setBg(mouse_pos - bl, 0xFF00FF00, LayerPriority_Debug);
    for (vec2i p : path)
    {
        g_game.term->setBg(p - bl, 0xFFFF00FF, LayerPriority_Debug);
    }
#endif

    sstring top_bar;
    top_bar.appendf("Mx: %d %d Px: %d %d", mouse_pos.x, mouse_pos.y, map.player->pos.x, map.player->pos.y);
    sstring bottom_bar;
    bottom_bar.appendf("Turn: %d H: %d/%d", map.turn, map.player->health, map.player->max_health);

    g_game.term->fillBg(vec2i(0, 0), vec2i(49, 0), 0xFF000000, LayerPriority_UI - 1);
    g_game.term->write(vec2i(2, 0), bottom_bar.c_str(), 0xFFFFFFFF, LayerPriority_UI);
    g_game.term->fillBg(vec2i(0, g_game.h - 1), vec2i(49, g_game.h - 1), 0xFF101010, LayerPriority_UI - 1);
    g_game.term->write(vec2i(2, g_game.h - 1), top_bar.c_str(), 0xFFFFFFFF, LayerPriority_UI);

    {
        g_game.term->fillBg(vec2i(50, g_game.h - 6), vec2i(g_game.w - 1, g_game.h - 1), 0xFF000000, LayerPriority_UI);

    }

    switch (g_game.sidebar)
    {
    case SidebarUI::Character:
    {
        drawUIFrame(g_game.term, vec2i(50, 1), vec2i(g_game.w - 1, g_game.h - 7), "Character");

    } break;
    case SidebarUI::GameLog:
    {
        drawUIFrame(g_game.term, vec2i(50, 1), vec2i(g_game.w - 1, g_game.h - 7), "Log");

        int rows = g_game.h - 7;

        InfoLog& log = g_game.log;
        int j = 2;
        for (int i = (int) log.entries.size() - 1; i >= 0 && j < rows; --i)
        {
            auto e = log.entries[i];
            auto parts = smartSplit(e.msg, g_game.w - 52);
            for (int k = (int) parts.size() - 1; k >= 0 && j < rows; --k)
            {
                g_game.term->write(vec2i(102, j), parts[k].c_str(), e.color, LayerPriority_UI);
                j++;
            }
        }

    } break;
    }
    {
        g_game.term->fillBg(vec2i(50, 0), vec2i(g_game.w - 1, 0), 0xFF000000, LayerPriority_UI);
        if (drawButton(g_game.term, vec2i(52, 0), "Character", g_game.sidebar == SidebarUI::Character ? 0xFFFFFFFF : 0xFFB0B0B0))
        {
            g_game.sidebar = SidebarUI::Character;
        }
        if (drawButton(g_game.term, vec2i(58, 0), "Log", g_game.sidebar == SidebarUI::GameLog ? 0xFFFFFFFF : 0xFFB0B0B0))
        {
            g_game.sidebar = SidebarUI::GameLog;
        }
    }

    map.render(*g_game.term, map.player->pos);
}
