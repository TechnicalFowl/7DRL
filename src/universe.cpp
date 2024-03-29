#include "universe.h"

#include <vector>

#include "actor.h"
#include "game.h"
#include "map.h"
#include "ship.h"
#include "sound.h"
#include "procgen.h"

const char* UActorTypeNames[UActorTypeCount]
{
    "Player",
    "Asteroid",
    "CargoShip",
    "Torpedo",
    "PirateShip",
    "Station",
    "ShipWreck",
    "MilitaryStation",
};

struct SpawningPct
{
    float cargo;
    float pirate;
    float station;
    float military;
    float mil_station;
    float alient_remenant;
};
SpawningPct spawning_bands[5]
{
    { 0.5f, 0.68f, 0.98f, 1.00f, 0.0f, 0.0f },
    { 0.2f, 0.65f, 0.85f, 0.99f, 1.00f, 0.0f },
    { 0.1f, 0.4f, 0.5f, 0.9f, 1.0f, 0.0f },
    { 0.1f, 0.2f, 0.3f, 0.75f, 1.0f, 0.0f },
    { 0.0f, 0.05f, 0.15f, 0.8f, 0.95f, 1.0f },
};

bool isShipType(UActorType t)
{
    return t == UActorType::CargoShip || t == UActorType::Player || t == UActorType::Torpedo || t == UActorType::PirateShip;
}

UShip::~UShip()
{
    if (ship)
    {
        auto it = std::find(g_game.ships.begin(), g_game.ships.end(), ship);
        if (it != g_game.ships.end())
            g_game.ships.erase(it);
        delete ship->map;
        delete ship;
    }
    ship = nullptr;
}

void UShip::update(pcg32& rng)
{
    UActor::update(rng);

    if (ship)
    {
        if (ship->hull_integrity <= 0)
            dead = true;

        bool pdc_used[16]{ false };

        for (UTorpedo* t : g_game.universe->torpedoes)
        {
            if (t->target == id)
            {
                if ((pos - t->pos).length() > 35)
                {
                    continue;
                }
                bool has_pdc = false;
                int i = 0;
                for (PDC* p : ship->pdcs)
                {
                    i++;
                    if (p->status != ShipObject::Status::Active) continue;
                    if (p->rounds == 0) continue;
                    if (pdc_used[i]) continue;

                    if (this == g_game.uplayer)
                    {
                        playSound(SoundEffect::PDCFire);
                    }

                    std::vector<UTorpedo*> intermediates;
                    auto points = findRay(pos, t->pos);
                    bool blocked = false;
                    for (vec2i p : points)
                    {
                        auto it = g_game.universe->actors.find(p);
                        if (it.found)
                        {
                            if (it.value->type == UActorType::Torpedo)
                            {
                                intermediates.push_back((UTorpedo*)it.value);
                            }
                            else
                            {
                                blocked = true;
                                break;
                            }
                        }
                    }
                    if (blocked) continue;
                    u32 col = 0xFFFFFFFF;
                    if (type == UActorType::PirateShip) col = 0xFFFF0000;
                    else if (type == UActorType::CargoShip) col = 0xFF0000FF;
                    RailgunAnimation* anim = new RailgunAnimation(0xFFFFFFFF, getProjectileCharacter(getDirection(pos, t->pos + t->vel)));
                    for (vec2i p : points) anim->points.push_back(p);
                    pdc_used[i] = true;
                    p->rounds = scalar::max(0, p->rounds - g_game.rng.nextInt(25, 75));
                    has_pdc = true;
                    for (UTorpedo* torp : intermediates)
                    {
                        float distance = (torp->pos - pos).length();
                        float hit_chance = 1 / (p->firing_variance * distance);
                        if (g_game.rng.nextFloat() < hit_chance)
                        {
                            anim->hits.push_back(torp->pos);
                            torp->dead = true;
                            if (this == g_game.uplayer)
                            {
                                if (torp->target == g_game.uplayer->id) g_game.log.log("Incoming torpedo destroyted by point defences.");
                                else g_game.log.log("Collateral torpedo destroyted by point defences.");
                            }
                            else if (torp->source == g_game.uplayer->id) g_game.log.log("Torpedo destroyed by enemy point defences.");
                            break;
                        }
                        else
                            anim->misses.push_back(torp->pos);
                    }
                    if (g_game.show_universe)
                        g_game.uanimations.push_back(anim);
                    else
                        delete anim;
                }
                if (!has_pdc) break;
            }
        }
    }
}

bool UShip::fireTorpedo(vec2i target, int power)
{
    auto t = g_game.universe->actors.find(target);
    if (t.found)
    {
        bool found_weapon = false;
        for (TorpedoLauncher* r : ship->torpedoes)
        {
            if (r->status != ShipObject::Status::Active) continue;
            if (r->charge_time == 0 && r->torpedoes > 0)
            {
                found_weapon = true;
                r->torpedoes--;
                r->charge_time = 4;
                break;
            }
        }
        if (!found_weapon)
        {
            if (this == g_game.uplayer)
            {
                g_game.log.log("No Torpedo Launchers available.");
            }
            return false;
        }

        vec2i offs = direction(getDirection(pos, target));
        if (offs.zero()) offs = vec2i(1, 0);
        vec2i spawn_pos = pos - vel + offs * 2;

        UTorpedo* torp = new UTorpedo(spawn_pos, power);
        torp->source = id;
        torp->target = isShipType(t.value->type) ? t.value->id : 0;
        torp->target_pos = target;
        torp->vel = vel;
        g_game.universe->spawn(torp);
        if (this == g_game.uplayer)
        {
            g_game.log.log("Torpedo launched.");
            playSound(SoundEffect::TorpedoLaunch);
        }
        return true;
    }
    return false;
}

bool UShip::fireRailgun(vec2i target, int power)
{
    bool found_weapon = false;
    Railgun* weapon = nullptr;
    for (Railgun* r : ship->railguns)
    {
        if (r->status != ShipObject::Status::Active) continue;
        if (r->charge_time == 0 && r->rounds > 0)
        {
            found_weapon = true;
            weapon = r;
            r->rounds--;
            r->charge_time = r->recharge_time;
            break;
        }
    }
    if (!found_weapon)
    {
        if (this == g_game.uplayer)
        {
            g_game.log.log("No railguns available.");
        }
        return false;
    }

    if (this == g_game.uplayer)
    {
        playSound(SoundEffect::RailgunFire);
    }

    pcg32& rng = g_game.universe->rng;
    float firing_variance = weapon->firing_variance;
    bool hit_anything = false;
    std::vector<vec2i> steps = findRay(pos, pos + (target - pos) * int(100 / (target - pos).length()));
    RailgunAnimation* anim = new RailgunAnimation(0xFFFFFFFF, getProjectileCharacter(getDirection(pos, target)));
    for (vec2i s: steps)
    {
        anim->points.push_back(s);
        auto it = g_game.universe->actors.find(s);
        if (it.found)
        {
            float distance = (pos - s).length();
            float hit_chance = 1 / (firing_variance * distance);
            bool solid_target = false;
            switch (it.value->type)
            {
            case UActorType::Player:
            {
                if (rng.nextFloat() < hit_chance)
                {
                    g_game.log.logf("Railgun impact (%.0f%%).", hit_chance * 100);
                    solid_target = true;
                    anim->hits.push_back(s);
                    ((UShip*)it.value)->ship->railgun(vec2i(), power);
                    g_game.gameover_reason = "Railgun fire";
                    playSound(SoundEffect::RailgunImpact);
                }
                else
                {
                    g_game.log.logf("Railgun near miss (%.0f%%).", hit_chance * 100);
                    anim->misses.push_back(s);
                }
                hit_anything = true;
            } break;
            case UActorType::Asteroid:
            {
                anim->misses.push_back(s);
                solid_target = true;
            } break;
            case UActorType::CargoShip:
            case UActorType::PirateShip:
            {
                if (rng.nextFloat() < hit_chance)
                {
                    if (this == g_game.uplayer) g_game.log.logf("Target hit (%.0f%%).", hit_chance * 100);
                    solid_target = true;
                    anim->hits.push_back(s);
                    ((UShip*) it.value)->ship->railgun(vec2i(), power);
                    if (this == g_game.uplayer && ((UShip*)it.value)->ship->hull_integrity <= 0)
                        g_game.uplayer->ships_killed++;
                }
                else
                {
                    if (this == g_game.uplayer) g_game.log.logf("Target missed (%.0f%%).", hit_chance * 100);
                    anim->misses.push_back(s);
                }
                hit_anything = true;
            } break;
            case UActorType::Torpedo:
            {
                if (rng.nextFloat() < hit_chance * 0.25f)
                {
                    if (this == g_game.uplayer) g_game.log.logf("Torpedo shot down (%.0f%%).", hit_chance * 25);
                    anim->hits.push_back(s);
                }
                else
                {
                    if (this == g_game.uplayer) g_game.log.logf("Torpedo missed (%.0f%%).", hit_chance * 25);
                    anim->misses.push_back(s);
                }
                hit_anything = true;
            } break;
            }
            if (solid_target) break;
        }

    }
    if (!hit_anything && this == g_game.uplayer)
        g_game.log.log("Target missed.");

    if (g_game.show_universe)
        g_game.uanimations.push_back(anim);
    else
        delete anim;
    return true;
}

UCargoShip::UCargoShip(vec2i p)
    : UShip(UActorType::CargoShip, p)
{
    ship = generate("cargo", "cargo_ship");
    g_game.ships.push_back(ship);

    for (Actor* a : ship->map->actors)
    {
        switch (a->type)
        {
        case ActorType::Reactor:
        {
            Reactor* r = (Reactor*)a;
            r->capacity = 100000;
        } break;
        case ActorType::PDC:
        {
            PDC* r = (PDC*)a;
            r->status = ShipObject::Status::Active;
            r->firing_variance *= 4;
        } break;
        default: break;
        }
    }
}

void UCargoShip::update(pcg32& rng)
{
    UShip::update(rng);
    if (vel.length() < 2)
    {
        vel += vec2i(rng.nextInt(-1, 2), rng.nextInt(-1, 2));
    }
    float speed = vel.length();
    if (speed > 0)
    {
        for (int i = 1; i <= 3; ++i)
        {
            if (g_game.universe->hasActor(pos + vel * i))
            {
                vel = vec2i((int)round(vel.x / speed), (int)round(vel.y / speed));
                break;
            }
        }
    }
}

void UCargoShip::render(TextBuffer& buffer, vec2i origin)
{
    if (animating) return;
    buffer.setTile(pos - origin, 'C', 0xFFFFFFFF, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x800000FF, LayerPriority_Overlay);
    }
}

UPirateShip::UPirateShip(vec2i p, int c, u32 col)
    : UShip(UActorType::PirateShip, p)
    , character(c), color(col)
{
    ship = generate("pirate", "pirate_ship");
    for (Actor* a: ship->map->actors)
    {
        switch (a->type)
        {
        case ActorType::Reactor:
        {
            Reactor* r = (Reactor*) a;
            r->capacity = 100000;
        } break;
        case ActorType::Railgun:
        {
            Railgun* r = (Railgun*) a;
            r->status = ShipObject::Status::Active;
        } break;
        case ActorType::PDC:
        {
            PDC* r = (PDC*) a;
            r->status = ShipObject::Status::Active;
            if (character == 'M') r->firing_variance *= 2;
            else if (character == 'P') r->firing_variance *= 4;
        } break;
        default: break;
        }
    }
    g_game.ships.push_back(ship);
    if (character == 'A')
    {
        torp_max_reloads = 100;
        railgun_max_reloads = 100;
        railgun_power = 3;
        torpedo_power = 16;
        for (Actor* a : ship->map->actors)
        {
            if (a->type == ActorType::Reactor)
            {
                ((Reactor*)a)->capacity = 100000;
            }
        }
    }
    else if (character == 'M')
    {
        railgun_power = 2;
        torpedo_power = 8;
    }
}

bool UPirateShip::isTarget(UActor* actor)
{
    switch (actor->type)
    {
    case UActorType::Player:
        if (character == 'M') return !g_game.uplayer->ship->transponder_masked;
        if (character == 'P') return !has_bribed;
        return true;
    case UActorType::CargoShip:
        return character == 'P' || character == 'A';
    case UActorType::PirateShip:
        return ((UPirateShip*)actor)->character != character;
    default:
        return false;
    }
}

void UPirateShip::update(pcg32& rng)
{
    UShip::update(rng);

    float sensor_range = ship->scannerRange();

    if (target == 0)
    {
        // If no target, look for one every 10 turns
        if (check_for_target <= 0)
        {
            // Look for the closest visible ship within our sensor range
            float closest_distance = 999999;

            for (auto it : g_game.universe->actors)
            {
                if (isTarget(it.value))
                {
                    float dist = (pos - it.value->pos).length();
                    if (dist < closest_distance && dist < sensor_range && g_game.universe->isVisible(pos, it.value->pos))
                    {
                        closest_distance = dist;
                        target = it.value->id;
                        target_last_pos = it.value->pos;
                    }
                }
            }
            // if we didn't find a target, try again in 10 turns
            if (target == 0)
                check_for_target = 10;
        }
        else
            check_for_target--;
    }
    else
    {
        // If we have a target, check if it's still valid
        auto target_it = g_game.universe->actor_ids.find(target);
        if (!target_it.found)
        {
            target = 0;
            check_for_target = 5;
        }
        else
        {
            UShip* s = (UShip*) target_it.value;
            if (!isTarget(s))
            {
                target = 0;
                check_for_target = 5;
            }
            else
            {
                float dist = (pos - s->pos).length();
                if (dist < sensor_range && g_game.universe->isVisible(pos, s->pos))
                {
                    // If the target is visible and within sensor range, update its last known position and potentially fire upon it
                    target_last_pos = s->pos;
                    if (dist < 12 && rng.nextFloat() < 0.6f)
                    {
                        fireRailgun(s->pos, railgun_power);
                    }
                    else if (dist < 25 && rng.nextFloat() < 0.3f)
                    {
                        fireTorpedo(s->pos, torpedo_power);
                    }
                }
                else
                {
                    target = 0;
                    check_for_target = 5;
                }
            }
        }
    }

    if (target == 0)
    {
        // If no target, wander around
        if (vel.length() < 2)
        {
            vel += vec2i(rng.nextInt(-1, 2), rng.nextInt(-1, 2));
        }
        float speed = vel.length();
        if (speed > 0)
        {
            for (int i = 1; i <= 3; ++i)
            {
                if (g_game.universe->hasActor(pos + vel * i))
                {
                    vel = vec2i((int)round(vel.x / speed), (int)round(vel.y / speed));
                    break;
                }
            }
        }
    }
    else
    {
        auto target_it = g_game.universe->actor_ids.find(target);
        debug_assert(target_it.found);
        UShip* s = (UShip*)target_it.value;

        float speed = vel.length();
        vec2i dist = target_last_pos - pos;
        if (dist.length() > 8)
        {
            // If we're far from the target, move towards it
            vec2i rvel = vel - s->vel;

            int slowdown_x = (abs(rvel.x) * (abs(rvel.x) + 1)) / 2;
            if ((rvel.x < 0) != (dist.x < 0))
                vel.x += (dist.x < 0) ? -1 : 1;
            else if (dist.y != 0 && abs(dist.x) <= slowdown_x)
                vel.x += (vel.x < 0) ? 1 : -1;
            else if (abs(dist.x) > slowdown_x)
                vel.x += (vel.x < 0) ? -1 : 1;

            int slowdown_y = (abs(rvel.y) * (abs(rvel.y) + 1)) / 2;
            if ((vel.y < 0) != (dist.y < 0))
                vel.y += (dist.y < 0) ? -1 : 1;
            else if (dist.x != 0 && abs(dist.y) <= slowdown_y)
                vel.y += (vel.y < 0) ? 1 : -1;
            else if (abs(dist.y) > slowdown_y)
                vel.y += (vel.y < 0) ? -1 : 1;
        }
        else
        {
            // Slowdown
            if (vel.x < 0) vel.x++;
            else if (vel.x > 0) vel.x--;

            if (vel.y < 0) vel.y++;
            else if (vel.y > 0) vel.y--;
        }
    }

    if (torp_max_reloads > 0)
    {
        TorpedoLauncher* needs_reload = nullptr;
        for (TorpedoLauncher* torp : ship->torpedoes)
        {
            if (torp->torpedoes == 0)
            {
                needs_reload = torp;
                break;
            }
        }
        if (needs_reload)
        {
            torp_reload_cooldown--;
            if (torp_reload_cooldown <= 0)
            {
                torp_reload_cooldown = 10;
                needs_reload->torpedoes = 5;
                torp_max_reloads--;
            }
        }
    }
    if (railgun_max_reloads > 0)
    {
        Railgun* needs_reload = nullptr;
        for (Railgun* torp : ship->railguns)
        {
            if (torp->rounds == 0)
            {
                needs_reload = torp;
                break;
            }
        }
        if (needs_reload)
        {
            railgun_reload_cooldown--;
            if (railgun_reload_cooldown <= 0)
            {
                railgun_reload_cooldown = 10;
                needs_reload->rounds = 25;
                railgun_max_reloads--;
            }
        }
    }
}

void UPirateShip::render(TextBuffer& buffer, vec2i origin)
{
    if (animating) return;
    buffer.setTile(pos - origin, character, color, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x80FF8080, LayerPriority_Overlay);
    }
}

void UPlayer::update(pcg32& rng)
{
    UShip::update(rng);
    float speed = vel.length();
    if (speed > 0)
    {
        for (int i = 1; i <= 2; ++i)
        {
            if (g_game.universe->hasActor(pos + vel * i))
            {
                g_game.log.log("Proximity alert");
                break;
            }
        }
    }
}

void UPlayer::render(TextBuffer& buffer, vec2i origin)
{
    if (animating) return;
    buffer.setTile(pos - origin, '@', 0xFFFFFFFF, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x8000FF00, LayerPriority_Overlay);
    }
}

void UTorpedo::update(pcg32& rng)
{
    UShip::update(rng);
    vec2i tvel;
    if (target != 0)
    {
        auto it = g_game.universe->actor_ids.find(target);
        if (!it.found)
        {
            if (source == g_game.uplayer->id)
                g_game.log.log("Torpedo has self-destructed as it's target was lost.");
            dead = true;
            return;
        }
        debug_assert(isShipType(it.value->type));
        UShip* target = (UShip*)it.value;
        tvel = target->vel;
        target_pos = target->pos + target->vel;
    }
    float speed = vel.length();
    vec2i dist = target_pos - pos;

    vec2i rvel = vel - tvel;

    int slowdown_x = (abs(rvel.x) * (abs(rvel.x) + 1)) / 2;
    if ((rvel.x < 0) != (dist.x < 0))
        vel.x += (dist.x < 0) ? -1 : 1;
    else if (dist.y != 0 && abs(dist.x) <= slowdown_x)
        vel.x += (vel.x < 0) ? 1 : -1;
    else if (abs(dist.x) > slowdown_x)
        vel.x += (vel.x < 0) ? -1 : 1;

    int slowdown_y = (abs(rvel.y) * (abs(rvel.y) + 1)) / 2;
    if ((vel.y < 0) != (dist.y < 0))
        vel.y += (dist.y < 0) ? -1 : 1;
    else if (dist.x != 0 && abs(dist.y) <= slowdown_y)
        vel.y += (vel.y < 0) ? 1 : -1;
    else if (abs(dist.y) > slowdown_y)
        vel.y += (vel.y < 0) ? -1 : 1;
}

void UTorpedo::render(TextBuffer& buffer, vec2i origin)
{
    if (animating) return;
    u32 col = 0xFFFFFFFF;
    if (target == g_game.uplayer->id)
        col = 0xFFFF0000;
    else if (source == g_game.uplayer->id)
        col = 0xFF00FF00;
    buffer.setTile(pos - origin, '!', col, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x80FFFF00, LayerPriority_Overlay);
    }
}

void UAsteroid::render(TextBuffer& buffer, vec2i origin)
{
    float step = scalar::PIf / (4 * radius);
    for (float a = 0; a + step/2 < scalar::PIf * 2; a += step)
    {
        float r = (radius + (float) cos(a * 1.2 + sfreq) * radius);
        vec2i p = pos + vec2i((int) round(cos(a) * r), (int) round(sin(a) * r));
        for (int r0 = 0; r0 < r; ++r0)
            buffer.setTile(pos + vec2i((int)round(cos(a) * r0), (int)round(sin(a) * r0)) - origin, '0', inner_color, LayerPriority_Background-1);
        buffer.setTile(p - origin, getProjectileCharacter(rotate90(getDirection(a))), color, LayerPriority_Background);
    }
}

UStation::UStation(vec2i p)
    : UActor(UActorType::Station, p)
{
    upgrades = g_game.rng.nextUInt();
    repair_cost = g_game.rng.nextInt(3, 10);
    scrap_price = g_game.rng.nextInt(7, 15);
}

void UStation::update(pcg32& rng)
{
    UActor::update(rng);
}

void UStation::render(TextBuffer& buffer, vec2i origin)
{
    buffer.setTile(pos - origin, 'S', 0xFFFFFFFF, LayerPriority_Actors);
    buffer.setText((pos - origin - vec2i(1, 0)) * vec2i(2, 1) + vec2i(1, 0), Border_TeeRight, 0xFFFFFFFF, LayerPriority_Actors - 1);
    buffer.setText((pos - origin + vec2i(1, 0)) * vec2i(2, 1), Border_TeeLeft, 0xFFFFFFFF, LayerPriority_Actors - 1);
}

UShipWreck::UShipWreck(vec2i p)
    : UActor(UActorType::ShipWreck, p)
{
    scrap = g_game.rng.nextInt(50, 150);
}

void UShipWreck::update(pcg32& rng)
{
    UActor::update(rng);
}

void UShipWreck::render(TextBuffer& buffer, vec2i origin)
{
    buffer.setTile(pos - origin, '$', 0xFFFFFFFF, LayerPriority_Actors);
}

UMilitaryStation::UMilitaryStation(vec2i p)
    : UActor(UActorType::MilitaryStation, p)
{
}

void UMilitaryStation::update(pcg32& rng)
{
    UActor::update(rng);
    if (!g_game.universe) return;
    float dist = (g_game.uplayer->pos - pos).length();
    if (dist < 35 && !g_game.uplayer->ship->transponder_masked)
    {
        charge_time--;
        if (charge_time <= 0)
        {
            vec2i offs = direction(getDirection(pos, g_game.uplayer->pos));
            if (offs.zero()) offs = vec2i(1, 0);
            vec2i spawn_pos = pos + offs * 2;

            UTorpedo* torp = new UTorpedo(spawn_pos, 12);
            torp->source = id;
            torp->target = g_game.uplayer->id;
            torp->target_pos = g_game.uplayer->pos;
            torp->vel = vec2i(0, 0);
            g_game.universe->spawn(torp);
            charge_time = 5;
        }
    }
}

void UMilitaryStation::render(TextBuffer& buffer, vec2i origin)
{
    buffer.setTile(pos - origin, 'M', 0xFFFF5000, LayerPriority_Actors);
    buffer.setText((pos - origin - vec2i(1, 0)) * vec2i(2, 1) + vec2i(1, 0), Border_TeeRight, 0xFFFF5000, LayerPriority_Actors - 1);
    buffer.setText((pos - origin + vec2i(1, 0)) * vec2i(2, 1), Border_TeeLeft, 0xFFFF5000, LayerPriority_Actors - 1);
}

vec2i getOffset(int& i, int& x, int& y)
{
    y++;
    if (y > i)
    {
        y = -i;
        x++;
    }
    while (true)
    {
        for (; x <= i; x++)
        {
            for (; y <= i; y++)
            {
                if (abs(x) == i || abs(y) == i)
                    return vec2i(x, y);
            }
            y = -i;
        }
        i++;
        x = -i;
        y = -i;
    }
}

Universe::Universe()
{
}

Universe::~Universe()
{
    for (auto it : actor_ids)
    {
        delete it.value;
    }
}

bool Universe::isVisible(vec2i from, vec2i to)
{
    auto points = findRay(from, to);
    if (points.empty()) return true;
    points.pop_back(); // Don't check the last point, as it's the target
    for (vec2i p : points)
    {
        auto it = actors.find(p);
        if (it.found && it.value->type == UActorType::Asteroid)
            return false;
    }
    return true;
}

bool Universe::checkArea(vec2i pos, int radius)
{
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            if (actors.find(pos + vec2i(x, y)).found)
                return false;
        }
    }
    return true;
}

void Universe::move(UActor* a, vec2i d)
{
    debug_assert(isShipType(a->type));
    bool target_occupied = actors.find(a->pos + d).found;
    std::vector<vec2i> steps = findRay(a->pos, a->pos + d);
    bool warned_this_step = false;
    vec2i last = a->pos;
    for (vec2i p: steps)
    {
        auto it = actors.find(p);
        if (it.found)
        {
            UShip* pl = (UShip*) a;
            if (it.value->type == UActorType::Torpedo)
            {
                UTorpedo* t = (UTorpedo*) it.value;
                if (t->source != a->id)
                {
                    if (pl->ship)
                    {
                        float incoming = rng.nextFloat() * scalar::PIf * 2;
                        vec2f dir = vec2f(cos(incoming), sin(incoming));
                        pl->ship->explosion(dir, rng.nextFloat() * t->power + 10);
                    }
                    else
                    {
                        pl->dead = true;
                    }

                    if (a == g_game.uplayer)
                    {
                        g_game.log.log("Torpedo detonating on hull!");
                        g_game.gameover_reason = "Torpedo detonation";
                        playSound(SoundEffect::TorpedoImpact);
                    }
                    else
                    {
                        if (t->source == g_game.uplayer->id)
                        {
                            if (t->target == a->id)
                                g_game.log.log("Target ship hit.");
                            else
                                g_game.log.log("Torpedo detonated on unknown target.");
                            if (pl->ship->hull_integrity <= 0)
                                g_game.uplayer->ships_killed++;
                        }
                    }
                    t->dead = true;
                }
                else if (p.x == a->pos.x + d.x && p.y == a->pos.y + d.y)
                {
                    UActor* torp = it.value;
                    last = p;
                    vec2i p0 = findEmpty(p);
                    bool rem = actors.erase(p);
                    debug_assert(rem);
                    torp->pos = p0;
                    debug_assert(!actors.find(p0).found);
                    actors.insert(torp->pos, torp);
                    continue;
                }
            }
            else if (a->type == UActorType::Torpedo)
            {
                UTorpedo* at = (UTorpedo*) a;
                if (at->source != it.value->id)
                {
                    if (isShipType(it.value->type) && ((UShip*) it.value)->ship)
                    {
                        float incoming = rng.nextFloat() * scalar::PIf * 2;
                        vec2f dir = vec2f(cos(incoming), sin(incoming));
                        UShip* os = (UShip*)it.value;
                        os->ship->explosion(dir, rng.nextFloat() * at->power + 10);
                    }

                    if (it.value == g_game.uplayer)
                    {
                        g_game.log.log("Torpedo detonating on hull!");
                        g_game.gameover_reason = "Torpedo detonation";
                        playSound(SoundEffect::TorpedoImpact);
                    }
                    else
                    {
                        if (at->source == g_game.uplayer->id)
                        {
                            if (at->target == it.value->id)
                                g_game.log.log("Target ship hit.");
                            else
                                g_game.log.log("Torpedo detonated on unknown target.");
                            if (isShipType(it.value->type) && ((UShip*)it.value)->ship && ((UShip*)it.value)->ship->hull_integrity <= 0)
                                g_game.uplayer->ships_killed++;
                        }
                    }
                    at->dead = true;
                    break;
                }
            }
            else if (pl->vel.length() > 6)
            {
                if (a == g_game.uplayer)
                {
                    g_game.log.log("Impact alert! Significant damage sustained.");
                    g_game.gameover_reason = "High-speed collision";
                    playSound(SoundEffect::AsteroidImpact);
                }

                vec2f dir = vec2f(rng.nextFloat() - 0.5f, 2.0f).normalize();
                pl->ship->explosion(dir, rng.nextFloat() * 3 * pl->vel.length() + 5);
                pl->vel = vec2i(0, 0);
                break;
            }
            else if (it.value->type == UActorType::Asteroid)
            {
                if (pl->vel.length() > 2)
                {
                    if (a == g_game.uplayer)
                    {
                        g_game.log.log("Impact alert!");
                        g_game.gameover_reason = "Low-speed collision";
                        playSound(SoundEffect::AsteroidImpact);
                    }

                    vec2f dir = vec2f(rng.nextFloat() - 0.5f, 2.0f).normalize();
                    pl->ship->explosion(dir, rng.nextFloat() * 3 * pl->vel.length() + 5);
                }
                else if (!warned_this_step)
                {
                    if (a == g_game.uplayer) g_game.log.log("Collision avoidance activated!");
                    warned_this_step = true;
                }
                pl->vel = vec2i(0, 0);
                break;
            }
            else if (!target_occupied)
            {
                if (!warned_this_step)
                {
                    if (a == g_game.uplayer) g_game.log.log("Collision avoidance activated!");
                    warned_this_step = true;
                }
            }
            else
            {
                if (!warned_this_step)
                {
                    if (a == g_game.uplayer) g_game.log.log("Collision avoidance activated!");
                    warned_this_step = true;
                }
                pl->vel = vec2i(0, 0);
                break;
            }
        }
        else
        {
            last = p;
        }
    }
    if (a->pos != last)
    {
        if (g_game.uplayer)
        {
            if (g_game.show_universe && a != g_game.uplayer && (a->pos - g_game.uplayer->pos).length() < g_game.uplayer->ship->scannerRange())
            {
                ShipMoveAnimation* anim = new ShipMoveAnimation((UShip*)a, a->pos, last);
                g_game.uanimations.push_back(anim);
            }
        }

        bool rem = actors.erase(a->pos);
        debug_assert(rem);
        a->pos = last;
        debug_assert(!actors.find(last).found);
        actors.insert(a->pos, a);
    }
}

vec2i Universe::findEmpty(vec2i p0)
{
    vec2i p = p0;
    int i = 0;
    int x = 0;
    int y = 0;
    while (actors.find(p).found)
    {
        p = p0 + getOffset(i, x, y);
    }
    return p;
}

void Universe::spawn(UActor* a)
{
    vec2i p = findEmpty(a->pos);
    actors.insert(p, a);
    a->pos = p;

    if (a->type == UActorType::Asteroid)
    {
        UAsteroid* ast = (UAsteroid*)a;
        float step = scalar::PIf / (4 * ast->radius);
        for (float a = 0; a + step / 2 < scalar::PIf * 2; a += step)
        {
            float r = (ast->radius + (float) cos(a * 1.2 + ast->sfreq) * ast->radius);
            vec2i p = ast->pos + vec2i((int)round(cos(a) * r), (int)round(sin(a) * r));
            for (int r0 = 0; r0 < r; ++r0)
            {
                vec2i p0 = ast->pos + vec2i((int)round(cos(a) * r0), (int)round(sin(a) * r0));
                if (actors.find(p0).found) continue;
                actors.insert(p0, (UActor*) ast);
            }
            if (!actors.find(p).found)
                actors.insert(p, (UActor*)ast);
        }
    }
    else if (a->type == UActorType::Torpedo)
    {
        torpedoes.push_back((UTorpedo*) a);
    }
    a->id = next_actor++;
    actor_ids.insert(a->id, a);
}

void Universe::update(vec2i origin)
{
    universe_ticks++;
    std::vector<vec2i> refresh_regions;
    for (auto it : regions_generated)
    {
        vec2i center((it.key.x << 5) + 16, (it.key.y << 5) + 16);
        if ((center - origin).length() > 116.0f)
            refresh_regions.push_back(it.key);
    }
    for (vec2i r : refresh_regions) regions_generated.erase(r);

    for (int y = origin.y - 70; y < origin.y + 70; y += 32)
    {
        int ry = y >> 5;
        int band = y < 0 ? 0 : y / 500;
        if (band > 4) band = 4;
        for (int x = origin.x - 70; x < origin.x + 70; x += 32)
        {
            int rx = x >> 5;

            if (!regions_generated.find(vec2i(rx, ry)).found)
            {
                // Populate region
                for (int y0 = 0; y0 < 4; ++y0)
                {
                    for (int x0 = 0; x0 < 4; ++x0)
                    {
                        if (y < -300 || (rng.nextFloat() < 0.05f && (rx != 0 || ry != 0)))
                        {
                            static u32 band_colors[5]{ 0xFFFFFFFF, 0xFFBBF0FF, 0xFFFFFF80, 0xFFFF9900, 0xFFFF5500, };
                            static u32 band_icolors[5]{ 0xFF505050, 0xFF105050, 0xFF505010, 0xFF503010, 0xFF502000, };
                            u32 color = band_colors[band];
                            u32 icolor = band_icolors[band];
                            UAsteroid* a = new UAsteroid(vec2i((rx << 5) + (x0 << 3), (ry << 5) + (y0 << 3)), color, icolor);
                            a->sfreq = rng.nextFloat() * 10;
                            a->radius = 1.0f + rng.nextFloat() * 2.0f + rng.nextFloat() * rng.nextFloat() * 8.0f;
                            spawn(a);
                        }
                        else
                        {
                            if (rng.nextFloat() < 0.02f)
                            {
                                vec2i p = vec2i((rx << 5) + (x0 << 4) + 2, (ry << 5) + (y0 << 3));
                                if (checkArea(p, 1))
                                {
                                    SpawningPct& pcts = spawning_bands[band];
                                    float type = rng.nextFloat();
                                    if (type < pcts.cargo)
                                    {
                                        UCargoShip* s = new UCargoShip(p);
                                        spawn(s);
                                    }
                                    else if (type < pcts.pirate)
                                    {
                                        UPirateShip* s = new UPirateShip(p, 'P', 0xFFFF0000);
                                        spawn(s);
                                    }
                                    else if(type < pcts.station)
                                    {
                                        UStation* s = new UStation(p);
                                        spawn(s);
                                    }
                                    else if(type < pcts.military)
                                    {
                                        UPirateShip* s = new UPirateShip(p, 'M', 0xFFFF0000);
                                        spawn(s);
                                    }
                                    else if(type < pcts.mil_station)
                                    {
                                        UMilitaryStation* s = new UMilitaryStation(p);
                                        spawn(s);
                                    }
                                    else if(type < pcts.alient_remenant && !has_spawned_alien)
                                    {
                                        UPirateShip* s = new UPirateShip(p, 'A', 0xFFFF00FF);
                                        spawn(s);
                                    }
                                }
                            }
                        }
                    }
                }

                regions_generated.insert(vec2i(rx, ry), true);
            }
        }
    }

    float player_scanners = g_game.uplayer ? g_game.uplayer->ship->scannerRange() : 1000;
    std::vector<UShip*> moved;
    std::vector<UActor*> to_remove;
    for (auto it : actors)
    {
        UActor* a = it.value;
        if (a->pos != it.key)
        {
            debug_assert(a->type == UActorType::Asteroid);
            continue; // Proxy actor
        }
        if ((a->pos - origin).length() > 160.0f)
        {
            to_remove.push_back(a);
            continue;
        }
        else if ((a->pos - origin).length() > player_scanners && isShipType(a->type))
        {
            if (!lost_tracks.find(a->id).found)
            {
                lost_tracks.insert(a->id, ULostTrack(a->pos, ((UShip*)a)->vel, 0xFFFF0000, a->id));
            }
        }
        else if (a->type == UActorType::PirateShip)
        {
            UPirateShip* pirate = (UPirateShip*)a;
            if (!pirate->has_alerted)
            {
                pirate->has_alerted = true;
                g_game.log.log("[Alert] New hostile contact in sensor range!");
            }
        }
        a->update(rng);

        if (a->dead)
        {
            to_remove.push_back(a);
        }
        else if (isShipType(a->type))
        {
            moved.push_back((UShip*) a);
        }
    }
    for (UShip* a : moved)
    {
        if (a->dead) continue;
        if (!a->vel.zero())
        {
            move(a, a->vel);
            if (a->dead)
            {
                to_remove.push_back(a);
            }
        }
    }
    for (UActor* a : to_remove)
    {
        if (a->type == UActorType::Player) continue;
        bool rem = actors.erase(a->pos);
        debug_assert(rem);
        rem = actor_ids.erase(a->id);
        debug_assert(rem);
        if (a->type == UActorType::Asteroid)
        {
            // Remove proxies
            UAsteroid* ast = (UAsteroid*)a;
            int r2 = scalar::ceili(ast->radius * 2);
            for (int y = -r2; y <= r2; ++y)
            {
                for (int x = -r2; x <= r2; ++x)
                {
                    vec2i p = a->pos + vec2i(x, y);
                    auto it = actors.find(p);
                    if (it.found && it.value == a)
                        actors.erase(p);
                }
            }
        }
        else if (a->type == UActorType::CargoShip || a->type == UActorType::PirateShip)
        {
            if (a->dead)
            {
                UShipWreck* wreck = new UShipWreck(a->pos);
                spawn(wreck);
            }
            if (a->type == UActorType::PirateShip && ((UPirateShip*)a)->character == 'A')
            {
                if (a->dead)
                {
                    g_game.state = GameState::Victory;
                }
                else
                {
                    has_spawned_alien = false;
                }
            }
        }
        else if (a->type == UActorType::Torpedo)
        {
            torpedoes.erase(std::find(torpedoes.begin(), torpedoes.end(), a));
        }
        delete a;
    }

    for (auto it: lost_tracks)
    {
        auto actor_it = actor_ids.find(it.key);
        if (!actor_it.found || (it.value.pos - origin).length() > 160.0f || (it.value.pos - origin).length() < player_scanners || ((actor_it.value->pos - origin).length() <= player_scanners && isVisible(actor_it.value->pos, origin)))
        {
            lost_tracks.erase(it.key);
            continue;
        }
        it.value.pos += it.value.vel;
    }
}

void Universe::render(TextBuffer& buffer, vec2i origin)
{
    float pscanner = g_game.uplayer ? g_game.uplayer->ship->scannerRange() : 1000;
    vec2i bl = origin - vec2i((g_game.w - 30) / 2, g_game.h / 2);
    for (auto it : actors)
    {
        if (it.value->pos == it.key)
        {
#if 0
            auto lost = lost_tracks.find(it.value->id);
            if (lost.found)
            {
                buffer.setOverlay(lost.value.pos, lost.value.color, LayerPriority_Overlay);
            }
            else
#else
            if (it.value->type == UActorType::Asteroid || (it.value->pos - origin).length() < pscanner)
#endif
                it.value->render(buffer, bl);
        }
        // else
        //    buffer.setBg(it.key - bl, 0xFF303030, LayerPriority_Background);
    }
}
