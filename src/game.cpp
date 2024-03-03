#include "game.h"

#include <cstdarg>

#include "actor.h"
#include "map.h"
#include "procgen.h"
#include "ship.h"
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

char getProjectileCharacter(Direction dir)
{
    return " |-/||\\>-\\-^/<v+"[dir];
}

ProjectileAnimation::ProjectileAnimation(const vec2i& from, const vec2i& to, u32 color, int c)
    : from(from), to(to), color(color), character(c)
{
    points = g_game.current_level->findRay(from, to);
    if (character == 0)
        character = getProjectileCharacter(getDirection(from, to));
}

bool ProjectileAnimation::draw()
{
    if (points.empty())
        return false;

    vec2i bl = g_game.current_level->player->pos - vec2i(25, 22);
    g_game.term->setTile(points[0] - bl, character, color, LayerPriority_Particles);
    points.erase(points.begin());
    return !points.empty();
}

struct TestModal : Modal
{
    TestModal(): Modal(vec2i(4, 10), vec2i(30, 10), "Test Modal") {}

    void draw()
    {

        if (drawButton(g_game.term, vec2i(22, 12), "Confirm", 0xFFFFFFFF))
        {
            close = true;
        }
        if (drawButton(g_game.term, vec2i(34, 12), "Cancel", 0xFFFFFFFF))
        {
            close = true;
        }
    }
};

void initGame(int w, int h)
{
    g_game.w = w;
    g_game.h = h;
    g_game.term = new TextBuffer(w, h);

    g_game.log.log("Welcome.");

    {
        Registry& reg = g_game.reg;
        reg.terrain_info[int(Terrain::Empty)] = TerrainInfo(Terrain::Empty, "Empty", TileEmpty, 0, 0xFF000000, true);
        reg.terrain_info[int(Terrain::ShipWall)] = TerrainInfo(Terrain::ShipWall, "Wall", TileFull, 0xFFD0D0D0, 0, false);
        reg.terrain_info[int(Terrain::ShipFloor)] = TerrainInfo(Terrain::ShipFloor, "Floor", TileFull, 0, 0xFF303030, true);

        reg.actor_info[int(ActorType::Player)] = ActorInfo(ActorType::Player, "Player", '@', 0xFFFFFFFF, LayerPriority_Actors + 10, false, 10);
        reg.actor_info[int(ActorType::GroundItem)] = ActorInfo(ActorType::GroundItem, "Item", '?', 0xFFFF00FF, LayerPriority_Objects + 1, true, 999);
        reg.actor_info[int(ActorType::InteriorDoor)] = ActorInfo(ActorType::InteriorDoor, "Interior Door", '#', 0xFFC0C0C0, LayerPriority_Objects, true, 50);
        reg.actor_info[int(ActorType::Airlock)] = ActorInfo(ActorType::Airlock, "Airlock", '#', 0xFFA0A0A0, LayerPriority_Objects, true, 50);
        reg.actor_info[int(ActorType::PilotSeat)] = ActorInfo(ActorType::PilotSeat, "Pilot Seat", 'P', 0xFFFFFFFF, LayerPriority_Objects, false, 50);
        reg.actor_info[int(ActorType::Engine)] = ActorInfo(ActorType::Engine, "Main Engine", 'E', 0xFFFFFFFF, LayerPriority_Objects, false, 50);
        reg.actor_info[int(ActorType::Reactor)] = ActorInfo(ActorType::Reactor, "Reactor", 'R', 0xFFFFFFFF, LayerPriority_Objects, false, 50);
        reg.actor_info[int(ActorType::Antenna)] = ActorInfo(ActorType::Antenna, "Antenna", 'A', 0xFFFFFFFF, LayerPriority_Objects, false, 50);
        reg.actor_info[int(ActorType::Scanner)] = ActorInfo(ActorType::Scanner, "Scanner", 'S', 0xFFFFFFFF, LayerPriority_Objects, false, 50);

        reg.actor_info[int(ActorType::TorpedoLauncher)] = ActorInfo(ActorType::TorpedoLauncher, "Torpedo Launcher", '!', 0xFFFFFFFF, LayerPriority_Objects, false, 50);
        reg.actor_info[int(ActorType::PDC)] = ActorInfo(ActorType::PDC, "Point Defence Cannon", 'D', 0xFFFFFFFF, LayerPriority_Objects, false, 50);
        reg.actor_info[int(ActorType::Railgun)] = ActorInfo(ActorType::Railgun, "Railgun", '%', 0xFFFFFFFF, LayerPriority_Objects, false, 50);

        reg.item_type_info[int(ItemType::Generic)] = ItemTypeInfo(ItemType::Generic, "Generic", '?', 0xFFFFFFFF);
        reg.item_type_info[int(ItemType::PhasarRifle)] = ItemTypeInfo(ItemType::PhasarRifle, "Phasar Rifle", ')', 0xFFFFFFFF);
        reg.item_type_info[int(ItemType::RepairParts)] = ItemTypeInfo(ItemType::RepairParts, "Repair Parts", '&', 0xFFFFFFFF);
        reg.item_type_info[int(ItemType::Torpedoes)] = ItemTypeInfo(ItemType::Torpedoes, "Torpedoes", ';', 0xFFFFFFFF);
        reg.item_type_info[int(ItemType::WeldingTorch)] = ItemTypeInfo(ItemType::WeldingTorch, "Welding Torch", '*', 0xFFFFFFFF);
        reg.item_type_info[int(ItemType::RailgunRounds)] = ItemTypeInfo(ItemType::RailgunRounds, "Railgun Rounds", ':', 0xFFFFFFFF);
        reg.item_type_info[int(ItemType::PDCRounds)] = ItemTypeInfo(ItemType::PDCRounds, "PDC Rounds", '"', 0xFFFFFFFF);
    }

    g_game.player_ship = generate("player", "player_ship");
    g_game.current_level = g_game.player_ship->map;
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
    term->fillBg(vec2i(min.x, min.y), vec2i(max.x, min.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x, min.y), vec2i(min.x, max.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x, max.y), vec2i(max.x, max.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(max.x, min.y), vec2i(max.x, max.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x + 1, min.y + 1), vec2i(max.x - 1, max.y - 1), 0xFF101010, LayerPriority_UI - 2);
    int title_len = (int) strlen(title);
    term->fillText(vec2i(min.x * 2, max.y), vec2i(min.x * 2 + 6, max.y), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
    term->fillText(vec2i(min.x * 2 + 9 + title_len, max.y), vec2i(max.x*2 + 1, max.y), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
    term->write(vec2i(min.x * 2 + 8, max.y), title, 0xFFA0A0A0, LayerPriority_UI - 1);
}

bool drawButton(TextBuffer* term, vec2i pos, const char* label, u32 color)
{
    sstring text; text.appendf("[%s]", label);

    vec2f mouse = screen_mouse_pos();
    bool hovered = mouse.x >= pos.x / 2.0f && mouse.x < (pos.x + text.size()) / 2.0f && scalar::floori(mouse.y) == pos.y;

    term->write(vec2i(pos.x, pos.y), text.c_str(), hovered ? 0xFF00FF00 : color, LayerPriority_UI);
    
    return hovered && input_mouse_button_pressed(GLFW_MOUSE_BUTTON_LEFT);
}

void updateGame()
{
    if (input_key_pressed(GLFW_KEY_F9))
    {
        delete g_game.player_ship;
        delete g_game.current_level;
        g_game.player_ship = generate("player", "player_ship");
        g_game.current_level = g_game.player_ship->map;
    }

    Map& map = *g_game.current_level;
    vec2i mouse_pos = game_mouse_pos();

    if (g_game.console_input_displayed)
    {
        InputTextResult res = input_handle_text(g_game.console_input, g_game.console_cursor);
        if (res != InputTextResult::Continue)
        {
            if (res == InputTextResult::Finished)
            {
                std::vector<sstring> args = g_game.console_input.split(' ');
            }
            g_game.console_input = "";
            g_game.console_cursor = 0;
            g_game.console_input_displayed = false;
        }
    }
    else if (!g_game.modal)
    {
        bool do_turn = map.player->next_action.action != Action::Wait;
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
            map.player->next_action = ActionData(Action::Open, map.player, 1.0f);
        }
        if (input_key_pressed(GLFW_KEY_U))
        {
            do_turn = true;
            map.player->next_action = ActionData(Action::UseOn, map.player, 1.0f);
        }
        if (input_key_pressed(GLFW_KEY_3) && (input_key_down(GLFW_KEY_LEFT_SHIFT) || input_key_down(GLFW_KEY_RIGHT_SHIFT)))
        {
            g_game.console_input_displayed = true;
        }
        if (input_key_pressed(GLFW_KEY_COMMA))
        {
            do_turn = true;
            map.player->next_action = ActionData(Action::Pickup, map.player, 1.0f);
        }
        if (g_window.inputs.scroll.y != 0)
        {
            static u64 last_scroll = 0;
            if (g_window.frame_count - last_scroll > 10)
            {
                do_turn = true;
                map.player->next_action = ActionData(Action::Wait, map.player, 1.0f);
                last_scroll = g_window.frame_count;
            }
        }
#if 0
        if (input_key_pressed(GLFW_KEY_Z))
        {
            if (map.player->is_aiming)
            {
                vec2i target = mouse_pos;
                do_turn = true;
                map.player->next_action = ActionData(Action::Zap, map.player, 1.0f, target);
            }
            else if (map.player->equipment[int(EquipmentSlot::MainHand)]
                && map.player->equipment[int(EquipmentSlot::MainHand)]->type == ItemType::Weapon)
            {
                Weapon* w = (Weapon*) map.player->equipment[int(EquipmentSlot::MainHand)];
                if (w->weapon_type == WeaponType::Ranged)
                {
                    map.player->is_aiming = true;
                }
            }
        }
        if (input_mouse_button_pressed(GLFW_MOUSE_BUTTON_1))
        {
            if (map.player->is_aiming)
            {
                vec2i target = mouse_pos;
                do_turn = true;
                map.player->next_action = ActionData(Action::Zap, map.player, 1.0f, target);
            }
        }
#endif
        // @TODO: ? opens help modal

        if (g_game.animations.empty() && do_turn)
        {
            float dt = map.player->next_action.energy;

            std::vector<ActionData> actions;
            for (Actor* a : map.actors)
            {
                // @Todo: Allow polling multiple actions per turn if the actor has enough stored energy?
                ActionData act = a->update(map, g_game.rng, dt);
                if (act.action != Action::Wait) actions.emplace_back(act);
            }

            for (ActionData& act : actions)
            {
                if (act.actor->dead) continue;
                act.apply(g_game.player_ship, g_game.rng);
                if (act.actor->type == ActorType::Player)
                {
                    ((Player*) act.actor)->is_aiming = false;
                }
            }
            for (auto it = map.actors.begin(); it != map.actors.end();)
            {
                if ((*it)->dead)
                {
                    ActorInfo& ai = g_game.reg.actor_info[int((*it)->type)];
                    auto tile_it = map.tiles.find((*it)->pos);
                    if (tile_it.found)
                    {
                        TerrainInfo& ti = g_game.reg.terrain_info[(int)tile_it.value.terrain];
                        if (ai.is_ground)
                        {
                            debug_assert(tile_it.value.ground == *it);
                            tile_it.value.ground = nullptr;
                        }
                        else
                        {
                            debug_assert(tile_it.value.actor == *it);
                            tile_it.value.actor = nullptr;
                        }
                    }
                    delete *it;
                    it = map.actors.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            map.turn++;
        }
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

    if (map.player->is_aiming)
    {
        vec2i bl = map.player->pos - vec2i(25, 22);
        auto path = map.findRay(map.player->pos, mouse_pos);
        for (vec2i p : path)
        {
            if (p == map.player->pos) continue;
            g_game.term->setOverlay(p - bl, 0x8000FF00, LayerPriority_Overlay);
        }
    }

    sstring top_bar;
    top_bar.appendf("Mx: %d %d Px: %d %d", mouse_pos.x, mouse_pos.y, map.player->pos.x, map.player->pos.y);
    g_game.term->fillBg(vec2i(0, g_game.h - 1), vec2i(49, g_game.h - 1), 0xFF101010, LayerPriority_UI - 1);
    g_game.term->write(vec2i(2, g_game.h - 1), top_bar.c_str(), 0xFFFFFFFF, LayerPriority_UI);

    sstring bottom_bar;
    if (g_game.console_input_displayed)
    {
        bottom_bar.appendf("# %s", g_game.console_input);
        if (g_window.frame_count % 30 < 15)
            g_game.term->setBg(vec2i(1 + g_game.console_cursor / 2, 0), 0xFFA0A0A0, LayerPriority_UI - 1);
    }
    else
    {
        bottom_bar.appendf("Turn: %d H: %d/%d", map.turn, map.player->health, map.player->max_health);
    }
    g_game.term->fillBg(vec2i(0, 0), vec2i(49, 0), 0xFF101010, LayerPriority_UI - 2);
    g_game.term->write(vec2i(2, 0), bottom_bar.c_str(), 0xFFFFFFFF, LayerPriority_UI);

    g_game.term->fillBg(vec2i(50, 0), vec2i(g_game.w - 1, g_game.h - 1), 0xFF101010, LayerPriority_UI - 2);
    {
        // Log
        int y0 = 8;
        g_game.term->setText(vec2i(100, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(101, y0), vec2i(106, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->setText(vec2i(107, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->write(vec2i(109, y0), "Log", 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->setText(vec2i(113, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(114, y0), vec2i(g_game.w * 2 - 2, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->setText(vec2i(g_game.w * 2 - 1, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(100, 0), vec2i(100, y0 - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(g_game.w * 2 - 1, 0), vec2i(g_game.w * 2 - 1, y0 - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);

        int rows = 8;

        InfoLog& log = g_game.log;
        int j = 0;
        for (int i = (int)log.entries.size() - 1; i >= 0 && j < rows; --i)
        {
            auto& e = log.entries[i];
            auto parts = smartSplit(e.msg, (g_game.w - 52) * 2);
            for (int k = (int)parts.size() - 1; k >= 0 && j < rows; --k)
            {
                g_game.term->write(vec2i(102, j), parts[k].c_str(), e.color, LayerPriority_UI);
                j++;
            }
        }
    }
    {
        int y0 = g_game.h - 5;
        g_game.term->setText(vec2i(100, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(101, y0), vec2i(106, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->setText(vec2i(107, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->write(vec2i(109, y0), "Ship", 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->setText(vec2i(114, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(115, y0), vec2i(g_game.w * 2 - 2, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->setText(vec2i(g_game.w * 2 - 1, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(100, 9), vec2i(100, y0 - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(g_game.w * 2 - 1, 9), vec2i(g_game.w * 2 - 1, y0 - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);

    }
    {
        sstring line_0;
        line_0.appendf("Holding: %s", map.player->holding ? map.player->holding->name.c_str() : "nothing");
        g_game.term->write(vec2i(102, g_game.h - 4), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        g_game.term->fillText(vec2i(100, g_game.h - 4), vec2i(100, g_game.h - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.term->fillText(vec2i(g_game.w * 2 - 1, g_game.h - 4), vec2i(g_game.w * 2 - 1, g_game.h - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
    }

    if (g_game.modal)
    {
        if (g_game.modal->draw_border)
            drawUIFrame(g_game.term, g_game.modal->pos, g_game.modal->pos + g_game.modal->size, g_game.modal->title.c_str());

        g_game.modal->draw();

        if (g_game.modal->close)
        {
            delete g_game.modal;
            g_game.modal = nullptr;
        }
    }

    for (auto it = g_game.animations.begin(); it != g_game.animations.end();)
    {
        Animation* a = *it;
        if (!a->draw())
            it = g_game.animations.erase(it);
        else
            ++it;
    }

    map.render(*g_game.term, map.player->pos);
}
