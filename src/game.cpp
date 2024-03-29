#include "game.h"

#include <cstdarg>

#include "actor.h"
#include "map.h"
#include "procgen.h"
#include "ship.h"
#include "universe.h"
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

Animation::Animation() : tick_created(g_game.current_level->turn) {}

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

    vec2i bl = g_game.current_level->player->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    g_game.mapterm->setTile(points[0] - bl, character, color, LayerPriority_Particles);
    points.erase(points.begin());
    return !points.empty();
}

RailgunAnimation::RailgunAnimation(u32 color, int c)
    : color(color), character(c)
{
}

bool RailgunAnimation::draw()
{
    vec2i bl = g_game.uplayer->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    if (!hits.empty() && hits[0] == last)
    {
        g_game.mapterm->setTile(hits[0] - bl, 'X', 0xFFFF0000, LayerPriority_Particles + 1);
        hits.erase(hits.begin());
    }
    else if (!misses.empty() && misses[0] == last)
    {
        g_game.mapterm->setTile(misses[0] - bl, 'X', 0xFFFFFFFF, LayerPriority_Particles + 1);
        misses.erase(misses.begin());
    }

    if (points.empty())
        return false;

    vec2i p = points[0] - bl;
    if (p.x < -2 || p.y < -2 || p.x >= g_game.w + 2 || p.y >= g_game.h + 2)
    {
        points.clear();
        return false;
    }
    g_game.mapterm->setTile(p, character, color, LayerPriority_Particles);
    last = points[0];
    points.erase(points.begin());
    return !points.empty() || !misses.empty() || !hits.empty();
}

ExplosionAnimation::ExplosionAnimation(vec2i c, int r)
    : center(c), radius(r)
{

}

bool ExplosionAnimation::draw()
{
    if (step > radius + 2)
        return false;

    step += 0.5f;
    vec2i bl = g_game.current_level->player->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    for (float a = 0; a < 2 * scalar::PIf; a += scalar::PIf / (3 * step))
    {
        vec2i p = center + vec2i(int(cos(a) * step), int(sin(a) * step));
        std::vector<vec2i> r = findRay(p, center);
        if (step < radius)
            g_game.mapterm->setOverlay(p - bl, 0xA0FF8000, LayerPriority_Particles+1);
        for (int i = 0; i < 3 && i < r.size(); ++i)
        {
            if (step - i - 1 < radius)
                g_game.mapterm->setOverlay(r[i] - bl, 0xD0D0D0D0, LayerPriority_Particles);
        }
    }
    return true;
}

ShipMoveAnimation::ShipMoveAnimation(UShip* s, vec2i f, vec2i t)
    : ship(s), from(f), to(t)
{
    points = findRay(f, t);
    switch (s->type)
    {
    case UActorType::Player:
        character = '@';
        color = 0xFFFFFFFF;
        break;
    case UActorType::CargoShip:
        character = 'C';
        color = 0xFFFFFFFF;
        break;
    case UActorType::PirateShip:
        character = 'P';
        color = 0xFFFF0000;
        break;
    case UActorType::Torpedo:
    {
        character = '!';
        UTorpedo* torp = (UTorpedo*)s;
        if (torp->target == g_game.uplayer->id)
            color = 0xFFFF0000;
        else if (torp->source == g_game.uplayer->id)
            color = 0xFF00FF00;
        else
            color = 0xFFFFFFFF;
    } break;
    }
    ship->animating = true;
}

ShipMoveAnimation::~ShipMoveAnimation()
{
    ship->animating = false;
}

bool ShipMoveAnimation::draw()
{
    if (points.empty())
        return false;

    vec2i bl = g_game.uplayer->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    vec2i p = points[0] - bl;
    if (p.x < -2 || p.y < -2 || p.x >= g_game.w + 2 || p.y >= g_game.h + 2)
    {
        points.clear();
        return false;
    }
    g_game.mapterm->setTile(p, character, color, LayerPriority_Particles);
    points.erase(points.begin());
    return !points.empty();
}

struct StationModal : Modal
{
    UStation* station;

    StationModal(UStation* s)
        : Modal(vec2i(4, 4), vec2i(40, g_game.h - 10), "Station")
        , station(s)
    {}

    void draw()
    {
        if (IsKeyPressed(KEY_ESCAPE))
        {
            close = true;
            return;
        }
        Ship* ps = g_game.player_ship;

        sstring credits_line; credits_line.appendf("Credits: %d", g_game.credits);
        g_game.uiterm->write(vec2i(12, 7), credits_line.c_str(), 0xFFFFFFFF, LayerPriority_UI + 1);

        sstring scrap_line; scrap_line.appendf("Scrap: %d", g_game.scrap);
        g_game.uiterm->write(vec2i(12, 9), scrap_line.c_str(), 0xFFFFFFFF, LayerPriority_UI + 1);

        int price = station->scrap_price;
        sstring scrap_sale; scrap_sale.appendf("Sell Scrap (%d credits)", price * g_game.scrap);
        if (drawButton(g_game.uiterm, vec2i(27, 9), scrap_sale, 0xFFFFFFFF, g_game.scrap == 0))
        {
            g_game.credits += price * g_game.scrap;
            g_game.scrap = 0;
            g_game.log.log("[Station] Thanks for your sale.");
        }

        int y0 = 11;

        u32 upgrades = station->upgrades;
        if (upgrades & 1)
        {
            int cost = station->repair_cost;
            sstring repair_line; repair_line.appendf("Repair (%d credits per point)", cost);
            g_game.uiterm->write(vec2i(12, y0), repair_line.c_str(), 0xFFFFFFFF, LayerPriority_UI + 1);
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 1), "Repair 10", 0xFFFFFFFF, g_game.credits < cost * 10 || ps->hull_integrity == ps->max_integrity))
            {
                g_game.credits -= cost * 10;
                g_game.uplayer->credits_spent += cost * 10;
                ps->repair(10);
                g_game.log.log("[Station] We have repaired some damage to your hull.");
            }
            if (drawButton(g_game.uiterm, vec2i(28, y0 + 1), "Repair 50", 0xFFFFFFFF, g_game.credits < cost * 50 || ps->hull_integrity >= ps->max_integrity - 10))
            {
                g_game.credits -= cost * 50;
                g_game.uplayer->credits_spent += cost * 50;
                ps->repair(50);
                g_game.log.log("[Station] We have repaired some damage to your hull.");
            }
            if (drawButton(g_game.uiterm, vec2i(48, y0 + 1), "Repair 100", 0xFFFFFFFF, g_game.credits < cost * 100 || ps->hull_integrity >= ps->max_integrity - 50))
            {
                g_game.credits -= cost * 100;
                g_game.uplayer->credits_spent += cost * 100;
                ps->repair(100);
                g_game.log.log("[Station] We have repaired some damage to your hull.");
            }
            y0 += 2;
        }
        else if (upgrades & 0x2)
        {
            int cost = 500 + 500 * (ps->max_integrity - 500) / 100;
            sstring line; line.appendf("Reinforce Hull (%d credits)", cost);
            if (drawButton(g_game.uiterm, vec2i(12, y0), line, 0xFFFFFFFF, g_game.credits < cost || ps->hull_integrity >= ps->max_integrity - 50))
            {
                g_game.credits -= cost;
                ps->max_integrity += 100;
                ps->hull_integrity += 100;
                g_game.uplayer->credits_spent += cost;
                g_game.log.log("[Station] Your hull has been reinforced.");
            }
            y0 += 2;
        }

        y0++;
        if (upgrades & 0x6)
        {
            int cost = 500 + 500 * (ps->reactor->capacity - 1000) / 1000;
            sstring line; line.appendf("Upgrade Reactor (%d credits)", cost);
            if (drawButton(g_game.uiterm, vec2i(12, y0), line, 0xFFFFFFFF, g_game.credits < cost))
            {
                g_game.credits -= cost;
                ps->reactor->capacity += 1000;
                g_game.uplayer->credits_spent += 1000;
                g_game.log.log("[Station] Your reactor capacity has been upgraded.");
            }
            y0++;
        }
        if (upgrades & 0xc)
        {
            Railgun* weapon_variance = nullptr;
            Railgun* weapon_charge = nullptr;
            Railgun* weapon_rounds = nullptr;
            for (Railgun* r : ps->railguns)
            {
                if (!weapon_variance || r->firing_variance > weapon_variance->firing_variance)
                    weapon_variance = r;
                if (!weapon_charge || r->recharge_time > weapon_charge->recharge_time)
                    weapon_charge = r;
                if (!weapon_rounds || r->max_rounds < weapon_rounds->max_rounds)
                    weapon_rounds = r;
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Upgrade Railgun Accuracy (500 credits)", 0xFFFFFFFF, !weapon_variance || g_game.credits < 500))
            {
                g_game.credits -= 500;
                g_game.uplayer->credits_spent += 500;
                weapon_variance->firing_variance /= 2;
                g_game.log.log("[Station] Your railgun accuracy has been upgraded.");
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 1), "Upgrade Railgun Charge Rate (500 credits)", 0xFFFFFFFF, !weapon_charge || weapon_charge->recharge_time == 0 || g_game.credits < 500))
            {
                g_game.credits -= 500;
                g_game.uplayer->credits_spent += 500;
                weapon_charge->recharge_time--;
                g_game.log.log("[Station] Your railgun charge rate has been upgraded.");
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 2), "Upgrade Railgun Max Rounds (500 credits)", 0xFFFFFFFF, !weapon_rounds || g_game.credits < 500))
            {
                g_game.credits -= 500;
                g_game.uplayer->credits_spent += 500;
                weapon_charge->max_rounds += 5;
                g_game.log.log("[Station] Your railgun magazine size has been upgraded.");
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 3), "Upgrade Railgun Power (1000 credits)", 0xFFFFFFFF, g_game.credits < 1000))
            {
                g_game.credits -= 1000;
                g_game.uplayer->credits_spent += 1000;
                g_game.uplayer->railgun_power++;
                g_game.log.log("[Station] Your railgun power has been upgraded.");
            }
            y0 += 4;
        }
        if (upgrades & 0x18)
        {
            TorpedoLauncher* weapon_count = nullptr;
            TorpedoLauncher* weapon_charge = nullptr;
            for (TorpedoLauncher* r : ps->torpedoes)
            {
                if (!weapon_count || r->max_torpedoes < weapon_count->max_torpedoes)
                    weapon_count = r;
                if (!weapon_charge || r->recharge_time > weapon_charge->recharge_time)
                    weapon_charge = r;
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Upgrade Torpedo Count (500 credits)", 0xFFFFFFFF, !weapon_count || g_game.credits < 500))
            {
                g_game.credits -= 500;
                g_game.uplayer->credits_spent += 500;
                weapon_count->max_torpedoes++;
                g_game.log.log("[Station] Your torpedo launcher magazine size has been upgraded.");
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 1), "Upgrade Torpedo Launcher Charge Rate (500 credits)", 0xFFFFFFFF, !weapon_charge || weapon_charge->recharge_time == 0 || g_game.credits < 500))
            {
                g_game.credits -= 500;
                g_game.uplayer->credits_spent += 500;
                weapon_charge->recharge_time--;
                g_game.log.log("[Station] Your torpedo launcher charge rate has been upgraded.");
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 2), "Upgrade Torpedo Explosive Power (1000 credits)", 0xFFFFFFFF, g_game.credits < 1000))
            {
                g_game.credits -= 1000;
                g_game.uplayer->credits_spent += 1000;
                g_game.uplayer->torpedo_power += g_game.rng.nextInt(3, 5);
                g_game.log.log("[Station] Your torpedo explosive power has been increased.");
            }
            y0 += 3;
        }
        if (upgrades & 0x30)
        {
            PDC* weapon_count = nullptr;
            PDC* weapon_charge = nullptr;
            for (PDC* r : ps->pdcs)
            {
                if (!weapon_count || r->firing_variance > weapon_count->firing_variance)
                    weapon_count = r;
                if (!weapon_charge || r->max_rounds < weapon_charge->max_rounds)
                    weapon_charge = r;
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Upgrade PDC Accuracy (500 credits)", 0xFFFFFFFF, !weapon_count || g_game.credits < 500))
            {
                g_game.credits -= 500;
                g_game.uplayer->credits_spent += 500;
                weapon_count->firing_variance /= 2;
                g_game.log.log("[Station] Your point defence accuracy has been upgraded.");
            }
            if (drawButton(g_game.uiterm, vec2i(12, y0 + 1), "Upgrade PDC Magazine Size (500 credits)", 0xFFFFFFFF, !weapon_charge || g_game.credits < 500))
            {
                g_game.credits -= 500;
                weapon_charge->max_rounds += 500;
                g_game.uplayer->credits_spent += 500;
                g_game.log.log("[Station] Your point defence magazine size has been upgraded.");
            }
            y0 += 2;
        }
        y0++;
        if (upgrades & 0x40)
        {
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Buy Repair Parts (50 credits / 5)", 0xFFFFFFFF, g_game.credits < 50))
            {
                ItemTypeInfo& ii = g_game.reg.item_type_info[int(ItemType::RepairParts)];
                Item* item = new Item(ii.character, ii.color, ItemType::RepairParts, ii.name, 5);
                if (ps->spawnItemInAirlock(item))
                {
                    g_game.credits -= 50;
                    g_game.uplayer->credits_spent += 50;
                    g_game.log.log("[Station] Repair parts transported to your airlock");
                }
                else
                {
                    delete item;
                    g_game.log.log("[Station] Please clear some space from your airlock to make room for your purchase");
                }
            }
            y0++;
        }
        if (upgrades & 0x80)
        {
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Buy Torpedoes (100 credits / 5)", 0xFFFFFFFF, g_game.credits < 100))
            {
                ItemTypeInfo& ii = g_game.reg.item_type_info[int(ItemType::Torpedoes)];
                Item* item = new Item(ii.character, ii.color, ItemType::Torpedoes, ii.name, 5);
                if (ps->spawnItemInAirlock(item))
                {
                    g_game.credits -= 100;
                    g_game.uplayer->credits_spent += 100;
                    g_game.log.log("[Station] Torpedoes transported to your airlock");
                }
                else
                {
                    delete item;
                    g_game.log.log("[Station] Please clear some space from your airlock to make room for your purchase");
                }
            }
            y0++;
        }
        if (upgrades & 0x100)
        {
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Buy Railgun Rounds (100 credits / 25)", 0xFFFFFFFF, g_game.credits < 100))
            {
                ItemTypeInfo& ii = g_game.reg.item_type_info[int(ItemType::RailgunRounds)];
                Item* item = new Item(ii.character, ii.color, ItemType::RailgunRounds, ii.name, 25);
                if (ps->spawnItemInAirlock(item))
                {
                    g_game.credits -= 100;
                    g_game.uplayer->credits_spent += 100;
                    g_game.log.log("[Station] Railgun Rounds transported to your airlock");
                }
                else
                {
                    delete item;
                    g_game.log.log("[Station] Please clear some space from your airlock to make room for your purchase");
                }
            }
            y0++;
        }
        if (upgrades & 0x200)
        {
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Buy PDC Rounds (100 credits / 500)", 0xFFFFFFFF, g_game.credits < 100))
            {
                ItemTypeInfo& ii = g_game.reg.item_type_info[int(ItemType::PDCRounds)];
                Item* item = new Item(ii.character, ii.color, ItemType::PDCRounds, ii.name, 500);
                if (ps->spawnItemInAirlock(item))
                {
                    g_game.credits -= 100;
                    g_game.uplayer->credits_spent += 100;
                    g_game.log.log("[Station] PDC Rounds transported to your airlock");
                }
                else
                {
                    delete item;
                    g_game.log.log("[Station] Please clear some space from your airlock to make room for your purchase");
                }
            }
            y0++;
        }

        y0++;
        if (!g_game.player_ship->transponder_masked && (upgrades & 0x80))
        {
            if (drawButton(g_game.uiterm, vec2i(12, y0), "Military Transponder code (1000 credits)", 0xFFFFFFFF, g_game.credits < 1000))
            {
                g_game.player_ship->transponder_masked = true;
                g_game.credits -= 1000;
                g_game.uplayer->credits_spent += 1000;
                g_game.log.log("[Station] If anyone asks you didn't buy this here.");
                g_game.log.log("Military Transponder code: 1234");
            }
            y0++;
        }

        if (drawButton(g_game.uiterm, vec2i(78, 6), "Undock", 0xFFFFFFFF))
        {
            close = true;
        }
    }
};

struct SalvageModal : Modal
{
    UShipWreck* wreck;

    SalvageModal(UShipWreck* s)
        : Modal(vec2i(4, 4), vec2i(40, g_game.h - 10), "Salvage")
        , wreck(s)
    {}

    void draw()
    {
        if (IsKeyPressed(KEY_ESCAPE))
        {
            close = true;
            return;
        }
        Ship* ps = g_game.player_ship;

        g_game.uiterm->write(vec2i(12, 7), "Select a salvage priority:", 0xFFFFFFFF, LayerPriority_UI + 1);
        if (drawButton(g_game.uiterm, vec2i(12, 8), "Scrap", 0xFFFFFFFF))
        {
            g_game.scrap += wreck->scrap;
            g_game.log.logf("Salvaged %d scrap.", wreck->scrap);
            g_game.uplayer->scrap_salvaged += wreck->scrap;
            wreck->dead = true;
            close = true;
        }

        if (drawButton(g_game.uiterm, vec2i(78, g_game.h - 7), "Leave", 0xFFFFFFFF))
        {
            close = true;
        }
    }
};

HailModal::HailModal(UShip* s)
    : Modal(vec2i(4, 4), vec2i(40, g_game.h - 10), "Ship Hail")
    , ship(s)
{
    was_masked = g_game.player_ship->transponder_masked;
}

void HailModal::draw()
{
    switch (ship->type)
    {
    case UActorType::CargoShip:
    {
        g_game.uiterm->write(vec2i(12, 7), "[Cargo Hauler] Careful, there are a lot of pirates around these parts.", 0xFFFFFFFF, LayerPriority_UI + 1);
        g_game.uiterm->write(vec2i(12, 7), "[Cargo Hauler] And there's some sort of military interdiction farther", 0xFFFFFFFF, LayerPriority_UI + 1);
        g_game.uiterm->write(vec2i(12, 7), "               up in the asteroid field.", 0xFFFFFFFF, LayerPriority_UI + 1);
    } break;
    case UActorType::PirateShip:
    {
        UPirateShip* pirate = (UPirateShip*) ship;
        if (pirate->character == 'P')
        {
            if (pirate->has_bribed)
            {
                g_game.uiterm->write(vec2i(12, 7), "[Pirate] Thanks for the credits.", 0xFFFFFFFF, LayerPriority_UI + 1);
                g_game.uiterm->write(vec2i(12, 8), "[Pirate] I'd leave now before I change my mind.", 0xFFFFFFFF, LayerPriority_UI + 1);
            }
            else
            {
                g_game.uiterm->write(vec2i(12, 7), "[Pirate] What do you want?", 0xFFFFFFFF, LayerPriority_UI + 1);
                if (drawButton(g_game.uiterm, vec2i(12, 9), "Bribe (100 credits)", 0xFFFFFFFF, g_game.credits < 100))
                {
                    g_game.credits -= 100;
                    g_game.uplayer->credits_spent += 100;
                    pirate->has_bribed = true;
                }
            }
        }
        else if (pirate->character == 'M')
        {
            if (was_masked)
            {
                g_game.uiterm->write(vec2i(12, 7), "[Military Ship] What? How did you get those transponder codes?", 0xFFFFFFFF, LayerPriority_UI + 1);
                g_game.uiterm->write(vec2i(12, 9), "(Your codes seem to have been invalidated)", 0xFFFFFFFF, LayerPriority_UI + 1);
                g_game.player_ship->transponder_masked = false;
            }
            else
            {
                g_game.uiterm->write(vec2i(12, 7), "[Military Ship] This area is under military control.", 0xFFFFFFFF, LayerPriority_UI + 1);
                g_game.uiterm->write(vec2i(12, 8), "[Military Ship] Leave at once or be fired upon.", 0xFFFFFFFF, LayerPriority_UI + 1);
            }
        }
        else if (pirate->character == 'A')
        {
            g_game.uiterm->write(vec2i(12, 7), "????", 0xFFFFFFFF, LayerPriority_UI + 1);
        }
        else
        {
            g_game.uiterm->write(vec2i(12, 7), "No response.", 0xFFFFFFFF, LayerPriority_UI + 1);
        }
    } break;
    default:
    {
        g_game.uiterm->write(vec2i(12, 7), "No response.", 0xFFFFFFFF, LayerPriority_UI + 1);
    } break;
    }

    if (drawButton(g_game.uiterm, vec2i(78, g_game.h - 7), "Leave", 0xFFFFFFFF))
    {
        close = true;
    }
}

void initGame(int w, int h)
{
    g_game.w = w;
    g_game.h = h;
    g_game.uiterm = new TextBuffer(w, h);
    g_game.mapterm = new TextBuffer(w, h);
    g_game.mapterm->invert = true;

    {
        Registry& reg = g_game.reg;
        reg.terrain_info[int(Terrain::Empty)] = TerrainInfo(Terrain::Empty, "Empty", TileEmpty, 0, 0xFF000000, true);
        reg.terrain_info[int(Terrain::ShipWall)] = TerrainInfo(Terrain::ShipWall, "Wall", TileFull, 0xFFD0D0D0, 0, false);
        reg.terrain_info[int(Terrain::ShipFloor)] = TerrainInfo(Terrain::ShipFloor, "Floor", TileFull, 0, 0xFF606060, true);
        reg.terrain_info[int(Terrain::DamagedShipFloor)] = TerrainInfo(Terrain::DamagedShipFloor, "Damaged Floor", TileFull, 0, 0xFF303030, true);
        reg.terrain_info[int(Terrain::DamagedShipWall)] = TerrainInfo(Terrain::DamagedShipWall, "Damaged Wall", TileFull, 0, 0xFF908080, false);

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
}

void startGame()
{
    g_game.log.entries.clear();
    g_game.log.log("Welcome.");

    if (g_game.universe)
    {
        delete g_game.universe;
    }
    g_game.ships.clear();
    g_game.universe = new Universe;
    g_game.player_ship = generate("player", "player_ship");
    g_game.current_level = g_game.player_ship->map;

    g_game.uplayer = new UPlayer(vec2i());
    g_game.uplayer->ship = g_game.player_ship;
    g_game.universe->spawn(g_game.uplayer);
    g_game.universe->update(g_game.uplayer->pos);

    g_game.player_ship->update();
    g_game.ships.push_back(g_game.player_ship);

    g_game.credits = 1000;
    g_game.scrap = 0;

    g_game.show_universe = false;
    g_game.transition = 0.0f;
    g_game.last_universe_update = -1000;

    g_game.modal = nullptr;
    g_game.animations.clear();
    g_game.uanimations.clear();
}

vec2i game_mouse_pos()
{
    vec2i bl = g_game.current_level->player->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    return vec2i(GetMouseX() / 16, g_game.h - GetMouseY() / 16) + bl;
}

vec2i universe_mouse_pos()
{
    vec2i bl = g_game.uplayer->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    return vec2i(GetMouseX() / 16, g_game.h - GetMouseY() / 16) + bl;
}

vec2f screen_mouse_pos()
{
    Vector2 mouse = GetMousePosition();
    return vec2f(mouse.x / 16, mouse.y / 16);
}

void drawUIFrame(TextBuffer* term, vec2i min, vec2i max, const char* title)
{
    term->fillBg(vec2i(min.x, min.y), vec2i(max.x, min.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x, min.y), vec2i(min.x, max.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x, max.y), vec2i(max.x, max.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(max.x, min.y), vec2i(max.x, max.y), 0xFF202020, LayerPriority_UI - 2);
    term->fillBg(vec2i(min.x + 1, min.y + 1), vec2i(max.x - 1, max.y - 1), 0xFF101010, LayerPriority_UI - 2);
    int title_len = (int) strlen(title);
    term->fillText(vec2i(min.x * 2, min.y), vec2i(min.x * 2 + 6, min.y), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
    term->fillText(vec2i(min.x * 2 + 9 + title_len, min.y), vec2i(max.x*2 + 1, min.y), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
    term->write(vec2i(min.x * 2 + 8, min.y), title, 0xFFA0A0A0, LayerPriority_UI - 1);
}

bool drawButton(TextBuffer* term, vec2i pos, const char* label, u32 color, bool disabled)
{
    sstring text; text.appendf("[%s]", label);

    vec2f mouse = screen_mouse_pos();
    bool hovered = mouse.x >= pos.x / 2.0f && mouse.x < (pos.x + text.size()) / 2.0f && scalar::floori(mouse.y) == pos.y;
    u32 col = hovered ? 0xFF00FF00 : color;
    if (disabled) col = 0xFF404040;

    term->write(vec2i(pos.x, pos.y), text.c_str(), col, LayerPriority_UI);
    
    return !disabled && hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void updateGame()
{
    bool do_map_turn = false;
    int gw = g_game.w - 30;

    if (IsKeyPressed(KEY_ESCAPE))
    {
        g_game.state = GameState::PauseMenu;
    }

    if (!g_game.modal && !g_game.show_universe && g_game.transition == 0)
    {
        Map& map = *g_game.current_level;
        do_map_turn = map.player->next_action.action != Action::Wait;
        if (IsKeyPressed(g_game.key_up))
        {
            do_map_turn = true;
            map.player->tryMove(map, vec2i(0, 1));
        }
        if (IsKeyPressed(g_game.key_down))
        {
            do_map_turn = true;
            map.player->tryMove(map, vec2i(0, -1));
        }
        if (IsKeyPressed(g_game.key_right))
        {
            do_map_turn = true;
            map.player->tryMove(map, vec2i(1, 0));
        }
        if (IsKeyPressed(g_game.key_left))
        {
            do_map_turn = true;
            map.player->tryMove(map, vec2i(-1, 0));
        }
        if (IsKeyPressed(g_game.key_open))
        {
            do_map_turn = true;
            map.player->next_action = ActionData(Action::Open, map.player, 1.0f);
        }
        if (IsKeyPressed(g_game.key_use))
        {
            do_map_turn = true;
            map.player->next_action = ActionData(Action::UseOn, map.player, 1.0f);
        }
        if (IsKeyPressed(g_game.key_pickup))
        {
            do_map_turn = true;
            map.player->next_action = ActionData(Action::Pickup, map.player, 1.0f);
        }
        if (IsKeyPressed(g_game.key_wait))
        {
            do_map_turn = true;
            map.player->next_action = ActionData(Action::Wait, map.player, 1.0f);
        }
        if (GetMouseWheelMove() != 0)
        {
            static u64 last_scroll = 0;
            if (g_window.frame_count - last_scroll > 10)
            {
                do_map_turn = true;
                map.player->next_action = ActionData(Action::Wait, map.player, 1.0f);
                last_scroll = g_window.frame_count;
            }
        }
#if 0
        if (IsKeyPressed(KEY_X))
        {
            vec2i center = (map.max + map.min) / 2;
            vec2i offset = game_mouse_pos() - center;
            vec2f dir = offset.cast<float>().normalize();
            g_game.player_ship->explosionAt(game_mouse_pos(), g_game.rng.nextFloat() * 20 + 10);
        }
#endif

#if 0
        if (IsKeyPressed(KEY_Z))
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
                Weapon* w = (Weapon*)map.player->equipment[int(EquipmentSlot::MainHand)];
                if (w->weapon_type == WeaponType::Ranged)
                {
                    map.player->is_aiming = true;
                }
            }
        }
        if (input_mouse_button_pressed(MOUSE_BUTTON_1))
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

        if (!g_game.animations.empty())
            do_map_turn = false;
    }
    else if (!g_game.modal && g_game.show_universe && g_game.transition == 0 && g_game.uanimations.empty())
    {
        // Clear ship animations if we're still in the universe view.
        g_game.animations.clear();

        bool engines_functional = false;
        for (MainEngine* e : g_game.player_ship->engines)
        {
            if (e->status == ShipObject::Status::Active)
            {
                engines_functional = true;
                break;
            }
        }

        bool do_turn = g_game.modal_close;
        if (IsKeyPressed(g_game.key_up))
        {
            if (!engines_functional)
            {
                g_game.log.log("Cannot move before repairing engines!");
            }
            else
            {
                do_turn = true;
                g_game.uplayer->vel += vec2i(0, 1);
            }
        }
        if (IsKeyPressed(g_game.key_down))
        {
            if (!engines_functional)
            {
                g_game.log.log("Cannot move before repairing engines!");
            }
            else
            {
                do_turn = true;
                g_game.uplayer->vel += vec2i(0, -1);
            }
        }
        if (IsKeyPressed(g_game.key_right))
        {
            if (!engines_functional)
            {
                g_game.log.log("Cannot move before repairing engines!");
            }
            else
            {
                do_turn = true;
                g_game.uplayer->vel += vec2i(1, 0);
            }
        }
        if (IsKeyPressed(g_game.key_left))
        {
            if (!engines_functional)
            {
                g_game.log.log("Cannot move before repairing engines!");
            }
            else
            {
                do_turn = true;
                g_game.uplayer->vel += vec2i(-1, 0);
            }
        }
        if (IsKeyPressed(g_game.key_open))
        {
            g_game.transition = 1.0f;
        }
        if (IsKeyPressed(g_game.key_railgun))
        {
            g_game.is_aiming_hail = false;
            g_game.uplayer->is_aiming = false;
            if (g_game.uplayer->is_aiming_railgun)
            {
                vec2i target = universe_mouse_pos();
                if (g_game.uplayer->fireRailgun(target, g_game.uplayer->railgun_power))
                {
                    do_turn = true;
                    g_game.uplayer->railgun_fired++;
                }
                g_game.uplayer->is_aiming_railgun = false;
            }
            else
            {
                g_game.uplayer->is_aiming_railgun = true;
            }
        }
        if (IsKeyPressed(g_game.key_fire))
        {
            g_game.is_aiming_hail = false;
            g_game.uplayer->is_aiming_railgun = false;
            if (g_game.uplayer->is_aiming)
            {
                vec2i target = universe_mouse_pos();
                if (g_game.uplayer->fireTorpedo(target, g_game.uplayer->torpedo_power))
                {
                    do_turn = true;
                    g_game.uplayer->torpedoes_launched++;
                }
                g_game.uplayer->is_aiming = false;
            }
            else
            {
                g_game.uplayer->is_aiming = true;
            }
        }
        if (IsKeyPressed(g_game.key_hail))
        {
            g_game.uplayer->is_aiming = false;
            g_game.uplayer->is_aiming_railgun = false;
            if (g_game.is_aiming_hail)
            {
                vec2i target = universe_mouse_pos();
                auto it = g_game.universe->actors.find(target);
                if (it.found && (it.value->type == UActorType::CargoShip || it.value->type == UActorType::PirateShip))
                {
                    g_game.modal = new HailModal((UShip*)it.value);
                }
                g_game.is_aiming_hail = false;
            }
            else
            {
                g_game.is_aiming_hail = true;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (g_game.uplayer->is_aiming)
            {
                vec2i target = universe_mouse_pos();
                if (g_game.uplayer->fireTorpedo(target, g_game.uplayer->torpedo_power))
                {
                    do_turn = true;
                    g_game.uplayer->torpedoes_launched++;
                }
                g_game.uplayer->is_aiming = false;
            }
            else if (g_game.uplayer->is_aiming_railgun)
            {
                vec2i target = universe_mouse_pos();
                if (g_game.uplayer->fireRailgun(target, g_game.uplayer->railgun_power))
                {
                    do_turn = true;
                    g_game.uplayer->railgun_fired++;
                }
                g_game.uplayer->is_aiming_railgun = false;
            }
            else if (g_game.is_aiming_hail)
            {
                vec2i target = universe_mouse_pos();
                auto it = g_game.universe->actors.find(target);
                if (it.found && (it.value->type == UActorType::CargoShip || it.value->type == UActorType::PirateShip))
                {
                    g_game.modal = new HailModal((UShip*)it.value);
                }
                g_game.is_aiming_hail = false;
            }
        }
        if (IsKeyPressed(g_game.key_wait))
        {
            do_turn = true;
        }
        if (IsKeyPressed(g_game.key_dock))
        {
            for (int i = 0; i < 4; ++i)
            {
                auto it = g_game.universe->actors.find(g_game.uplayer->pos + cardinals[i]);
                if (it.found)
                {
                    if (it.value->type == UActorType::Station)
                    {
                        UStation* station = (UStation*)it.value;
                        if (!station->has_visited)
                        {
                            station->has_visited = true;
                            g_game.uplayer->stations_visited++;
                        }
                        g_game.modal = new StationModal(station);
                        break;
                    }
                    else if (it.value->type == UActorType::ShipWreck)
                    {
                        g_game.modal = new SalvageModal((UShipWreck*) it.value);
                        break;
                    }
                    else if (it.value->type == UActorType::MilitaryStation)
                    {
                        if (g_game.player_ship->transponder_masked)
                        {
                            g_game.log.log("Attempting to dock with the military station reveals your faked transponder codes.");
                            g_game.player_ship->transponder_masked = false;
                        }
                    }
                }
            }
        }

        if (GetMouseWheelMove() != 0)
        {
            static u64 last_scroll = 0;
            if (g_window.frame_count - last_scroll > 10)
            {
                do_turn = true;
                last_scroll = g_window.frame_count;
            }
        }

        if (do_turn)
        {
            do_map_turn = true;
            g_game.modal_close = false;
            g_game.last_universe_update = g_game.current_level->turn;
            g_game.universe->update(g_game.uplayer->pos);
        }

        if (g_game.uplayer && g_game.uplayer->ship->pilot->status != ShipObject::Status::Active)
        {
            g_game.transition = 1.0f;
        }
    }

    if (do_map_turn)
    {
        float dt = g_game.current_level->player->next_action.energy;

        for (Ship* s : g_game.ships)
        {
            std::vector<ActionData> actions;

            for (Actor* a : s->map->actors)
            {
                // @Todo: Allow polling multiple actions per turn if the actor has enough stored energy?
                ActionData act = a->update(*s->map, g_game.rng, dt);
                if (act.action != Action::Wait) actions.emplace_back(act);
            }

            for (ActionData& act : actions)
            {
                if (act.actor->dead) continue;
                act.apply(s, g_game.rng);
                if (act.actor->type == ActorType::Player)
                {
                    ((Player*)act.actor)->is_aiming = false;
                }
            }

            for (auto it = s->map->actors.begin(); it != s->map->actors.end();)
            {
                if ((*it)->dead)
                {
                    ActorInfo& ai = g_game.reg.actor_info[int((*it)->type)];
                    auto tile_it = s->map->tiles.find((*it)->pos);
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
                    delete* it;
                    it = s->map->actors.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            s->map->turn++;
            s->update();
        }

        if (g_game.player_ship->map->turn > g_game.last_universe_update + 10)
        {
            g_game.last_universe_update = g_game.current_level->turn;
            g_game.universe->update(g_game.uplayer->pos);
        }
        if (g_game.player_ship->hull_integrity <= 0)
        {
            g_game.log.log("Your ship sustained too much damage and your hull breaks apart.");
            g_game.log.log("Game over.");
            g_game.state = GameState::GameOver;
        }
    }

    g_game.uiterm->clear(g_game.w, g_game.h);
    g_game.mapterm->clear(g_game.w, g_game.h);

#if 0
    // Pathfinding debug
    auto path = map.findPath(map.player->pos, mouse_pos);
    g_game.term->setBg(mouse_pos - bl, 0xFF00FF00, LayerPriority_Debug);
    for (vec2i p : path)
    {
        g_game.term->setBg(p - bl, 0xFFFF00FF, LayerPriority_Debug);
    }

    if (!g_game.show_universe && map.player->is_aiming)
    {
        vec2i bl = map.player->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
        auto path = map.findRay(map.player->pos, mouse_pos);
        for (vec2i p : path)
        {
            if (p == map.player->pos) continue;
            g_game.mapterm->setOverlay(p - bl, 0x8000FF00, LayerPriority_Overlay);
        }
    }
#endif
    if (g_game.show_universe)
    {
        if (g_game.uplayer->is_aiming || g_game.is_aiming_hail)
        {
            vec2i mouse_pos = universe_mouse_pos();
            vec2i bl = g_game.uplayer->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
            g_game.mapterm->setOverlay(mouse_pos - bl, 0x8000FF00, LayerPriority_Overlay);
        }
        else if (g_game.uplayer->is_aiming_railgun)
        {
            vec2i mouse_pos = universe_mouse_pos();
            vec2i bl = g_game.uplayer->pos - vec2i((g_game.w - 30) / 2, g_game.h / 2);
            std::vector<vec2i> steps = findRay(g_game.uplayer->pos, mouse_pos);
            for (vec2i s: steps)
                g_game.mapterm->setOverlay(s - bl, 0x8080FF00, LayerPriority_Overlay);
        }
    }

    sstring top_bar;
    if (g_game.show_universe)
    {
        vec2i mouse_pos = universe_mouse_pos();
        top_bar.appendf("Examine: %d %d", mouse_pos.x, mouse_pos.y);
        auto it = g_game.universe->actors.find(mouse_pos);
        if (it.found)
        {
            top_bar.appendf("   %s", UActorTypeNames[int(it.value->type)]);
        }
    }
    else
    {
        vec2i mouse_pos = game_mouse_pos();
        auto it = g_game.current_level->tiles.find(mouse_pos);
        top_bar.appendf("Examine: (%d %d)", mouse_pos.x, mouse_pos.y);
        if (it.found)
        {
            TerrainInfo& ti = g_game.reg.terrain_info[int(it.value.terrain)];
            top_bar.appendf("   %s", ti.name.c_str());
            if (it.value.actor)
            {
                ActorInfo& ai = g_game.reg.actor_info[int(it.value.actor->type)];
                top_bar.appendf("  %s", ai.name.c_str());
            }
            if (it.value.ground)
            {
                if (it.value.ground->type == ActorType::GroundItem)
                {
                    GroundItem* item = (GroundItem*)it.value.ground;
                    top_bar.appendf("  %s", item->item->getName().c_str());
                }
                else
                {
                    ActorInfo& ai = g_game.reg.actor_info[int(it.value.ground->type)];
                    top_bar.appendf("  %s", ai.name.c_str());
                }
            }
        }
        else
        {
            top_bar.append("   empty");
        }
    }
    g_game.uiterm->fillBg(vec2i(0, 0), vec2i(gw - 1, 0), 0xFF101010, LayerPriority_UI - 1);
    g_game.uiterm->write(vec2i(2, 0), top_bar.c_str(), 0xFFFFFFFF, LayerPriority_UI);

    sstring bottom_bar;
    sstring holding = g_game.current_level->player->holding ? g_game.current_level->player->holding->getName() : sstring("nothing");
    bottom_bar.appendf("Turn: %d/%d    View: %s    Holding: %s",
        g_game.current_level->turn, g_game.universe->universe_ticks,
        g_game.show_universe ? "Universe" : "Ship",
        holding.c_str());
    g_game.uiterm->fillBg(vec2i(0, g_game.h - 1), vec2i(gw - 1, g_game.h - 1), 0xFF101010, LayerPriority_UI - 2);
    g_game.uiterm->write(vec2i(2, g_game.h - 1), bottom_bar.c_str(), 0xFFFFFFFF, LayerPriority_UI);

    g_game.uiterm->fillBg(vec2i(gw, 0), vec2i(g_game.w - 1, g_game.h - 1), 0xFF101010, LayerPriority_UI - 2);
    {
        // Log
        int rows = g_game.h / 3;
        int y0 = g_game.h - rows;
        g_game.uiterm->setText(vec2i(gw * 2, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(gw * 2 + 1, y0), vec2i(gw * 2 + 6, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->setText(vec2i(gw * 2 + 7, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->write(vec2i(gw * 2 + 9, y0), "Log", 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->setText(vec2i(gw * 2 + 13, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(gw * 2 + 14, y0), vec2i(g_game.w * 2 - 2, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->setText(vec2i(g_game.w * 2 - 1, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(gw * 2, y0 + 1), vec2i(gw * 2, g_game.h - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(g_game.w * 2 - 1, y0 + 1), vec2i(g_game.w * 2 - 1, g_game.h - 1), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);

        InfoLog& log = g_game.log;
        int j = 1;
        for (int i = (int)log.entries.size() - 1; i >= 0 && j < rows; --i)
        {
            auto& e = log.entries[i];
            auto parts = smartSplit(e.msg, (g_game.w - gw - 2) * 2);
            for (int k = (int)parts.size() - 1; k >= 0 && j < rows; --k)
            {
                g_game.uiterm->write(vec2i(gw * 2 + 2, g_game.h - j), parts[k].c_str(), e.color, LayerPriority_UI);
                j++;
            }
        }
    }
    {
        int y0 = 5;
        g_game.uiterm->setText(vec2i(gw * 2, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(gw * 2 + 1, y0), vec2i(gw * 2 + 6, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->setText(vec2i(gw * 2 + 7, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->write(vec2i(gw * 2 + 9, y0), "Ship", 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->setText(vec2i(gw * 2 + 14, y0), Border_TeeRight, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(gw * 2 + 15, y0), vec2i(g_game.w * 2 - 2, y0), Border_Horizontal, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->setText(vec2i(g_game.w * 2 - 1, y0), Border_TeeLeft, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(gw * 2, 6), vec2i(gw * 2, g_game.h - 16), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(g_game.w * 2 - 1, 6), vec2i(g_game.w * 2 - 1, g_game.h - 16), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);

#if 1
        Ship* ps = g_game.player_ship;
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Hull: %d", ps->hull_integrity);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }

        if (ps->pilot)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Pilot [%s]", ShipObjectStatus[int(ps->pilot->status)]);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        if (ps->scanner)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Antenna [%s]", ShipObjectStatus[int(ps->pilot->status)]);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        if (ps->reactor)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Reactor [%s]", ShipObjectStatus[int(ps->reactor->status)]);
            if (ps->reactor->status == ShipObject::Status::Active)
                line_0.appendf(" %.0f/%.0f", ps->reactor->power, ps->reactor->capacity);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        for (MainEngine* e : ps->engines)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Engine [%s]", ShipObjectStatus[int(e->status)]);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        for (TorpedoLauncher* e : ps->torpedoes)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Torpedo: [%s]", ShipObjectStatus[int(e->status)]);
            if (ps->reactor->status == ShipObject::Status::Active)
            {
                line_0.appendf(" A: %d/%d", e->torpedoes, e->max_torpedoes);
                if (e->charge_time > 0)
                    line_0.appendf(" Ready-in: %d", e->charge_time);
            }
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        for (PDC* e : ps->pdcs)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Point Defence: [%s]", ShipObjectStatus[int(e->status)]);
            if (ps->reactor->status == ShipObject::Status::Active)
            {
                line_0.appendf(" A: %d/%d", e->rounds, e->max_rounds);
            }
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        for (Railgun* e : ps->railguns)
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Railgun: [%s]", ShipObjectStatus[int(e->status)]);
            if (ps->reactor->status == ShipObject::Status::Active)
            {
                line_0.appendf(" A: %d/%d", e->rounds, 25);
                if (e->charge_time > 0)
                    line_0.appendf(" Ready-in: %d", e->charge_time);
            }
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
#else
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Actors: %d", g_game.universe->actors.size());
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Player: %d %d", g_game.uplayer->pos.x, g_game.uplayer->pos.y);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }

        vec2i min_pos(99999, 99999);
        vec2i max_pos(-99999, -99999);

        int tcount[6]{ 0 };

        for (auto it : g_game.universe->actors)
        {
            UActor* a = it.value;
            tcount[int(a->type)]++;
            min_pos = min(min_pos, a->pos);
            max_pos = max(max_pos, a->pos);
            if (a->type == UActorType::Asteroid)
            {

            }
            else
            {
                debug_assert(it.key == it.value->pos);
            }
        }
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Min: %d %d Max: %d %d", min_pos.x, min_pos.y, max_pos.x, max_pos.y);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
        {
            ++y0;
            sstring line_0;
            line_0.appendf("Counts: %d %d %d %d %d %d", tcount[0], tcount[1], tcount[2], tcount[3], tcount[4], tcount[5]);
            g_game.uiterm->write(vec2i(gw * 2 + 2, y0), line_0.c_str(), 0xFFFFFFFF, LayerPriority_UI);
        }
#endif
    }
    {
        if (g_game.show_universe)
        {
            g_game.uiterm->write(vec2i(gw * 2 + 2, 0), "", 0xFFFFFFFF, LayerPriority_UI);
            g_game.uiterm->write(vec2i(gw * 2 + 2, 1), "arrows: move   space: wait", 0xFFFFFFFF, LayerPriority_UI);
            g_game.uiterm->write(vec2i(gw * 2 + 2, 2), "z: railgun    f: torpedoes", 0xFFFFFFFF, LayerPriority_UI);
            g_game.uiterm->write(vec2i(gw * 2 + 2, 3), "u: stop piloting   d: dock    h: hail", 0xFFFFFFFF, LayerPriority_UI);
        }
        else
        {
            g_game.uiterm->write(vec2i(gw * 2 + 2, 0), "", 0xFFFFFFFF, LayerPriority_UI);
            g_game.uiterm->write(vec2i(gw * 2 + 2, 1), "arrows: move    space: wait", 0xFFFFFFFF, LayerPriority_UI);
            g_game.uiterm->write(vec2i(gw * 2 + 2, 2), "", 0xFFFFFFFF, LayerPriority_UI);
            g_game.uiterm->write(vec2i(gw * 2 + 2, 3), "o: interact     u: use     comma: pickup", 0xFFFFFFFF, LayerPriority_UI);
        }
        g_game.uiterm->fillText(vec2i(gw * 2, 0), vec2i(gw * 2, 4), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
        g_game.uiterm->fillText(vec2i(g_game.w * 2 - 1, 0), vec2i(g_game.w * 2 - 1, 4), Border_Vertical, 0xFFA0A0A0, LayerPriority_UI - 1);
    }

    if (g_game.modal)
    {
        if (g_game.modal->draw_border)
            drawUIFrame(g_game.uiterm, g_game.modal->pos, g_game.modal->pos + g_game.modal->size, g_game.modal->title.c_str());

        g_game.modal->draw();

        if (g_game.modal->close)
        {
            delete g_game.modal;
            g_game.modal = nullptr;
            g_game.modal_close = true;
        }
    }
    if (g_game.transition == 0)
    {
        if (g_game.show_universe)
        {
            for (auto it = g_game.uanimations.begin(); it != g_game.uanimations.end();)
            {
                Animation* a = *it;
                if (!a->draw())
                {
                    it = g_game.uanimations.erase(it);
                    delete a;
                }
                else
                    ++it;
            }
        }
        else
        {
            for (auto it = g_game.animations.begin(); it != g_game.animations.end();)
            {
                Animation* a = *it;
                if (!a->draw())
                {
                    it = g_game.animations.erase(it);
                    delete a;
                }
                else
                    ++it;
            }
        }
    }

    if (g_game.transition > 0)
    {
        bool past_half = g_game.transition < 0.5f;
        if (past_half ^ g_game.show_universe)
            g_game.universe->render(*g_game.mapterm, g_game.uplayer->pos);
        else
            g_game.current_level->render(*g_game.mapterm, g_game.current_level->player->pos);
        g_game.transition -= 0.02f;
        if (g_game.transition <= 0)
        {
            g_game.show_universe = !g_game.show_universe;
            g_game.transition = 0;
            g_window.map_zoom = 1.0f;
        }
        else
        {
            if (g_game.show_universe)
                g_window.map_zoom = past_half ? 1 - g_game.transition * 2 : 1 / ((g_game.transition - 0.5f) * 2);
            else
                g_window.map_zoom = past_half ? 1 / (1 - g_game.transition * 2) : (g_game.transition - 0.5f) * 2;
        }
        if (g_window.map_zoom < 1e-6) g_window.map_zoom = 0.001f;
    }
    else if (g_game.show_universe)
    {
        g_game.universe->render(*g_game.mapterm, g_game.uplayer->pos);
    }
    else
    {
        g_game.current_level->render(*g_game.mapterm, g_game.current_level->player->pos);
    }
}
