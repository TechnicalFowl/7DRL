#include "universe.h"

#include <vector>

#include "actor.h"
#include "game.h"
#include "map.h"
#include "ship.h"
#include "procgen.h"

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
    if (ship && ship->hull_integrity < 0)
        dead = true;
}

bool UShip::fireTorpedo(vec2i target)
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
        vec2i spawn_pos = pos + vel + offs;

        UTorpedo* torp = new UTorpedo(spawn_pos);
        torp->source = id;
        torp->target = isShipType(t.value->type) ? t.value->id : 0;
        torp->target_pos = target;
        torp->vel = vel;
        g_game.universe->spawn(torp);
        if (this == g_game.uplayer) g_game.log.log("Torpedo launched.");
        return true;
    }
    return false;
}

bool UShip::fireRailgun(vec2i target)
{
    bool found_weapon = false;
    for (Railgun* r : ship->railguns)
    {
        if (r->status != ShipObject::Status::Active) continue;
        if (r->charge_time == 0 && r->rounds > 0)
        {
            found_weapon = true;
            r->rounds--;
            r->charge_time = 4;
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

    pcg32& rng = g_game.universe->rng;
    float firing_variance = scalar::PIf / 20;
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
                    g_game.log.log("Railgun impact.");
                    solid_target = true;
                    anim->hits.push_back(s);
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
            {
                if (rng.nextFloat() < hit_chance)
                {
                    if (this == g_game.uplayer) g_game.log.logf("Target hit (%.0f%%).", hit_chance * 100);
                    solid_target = true;
                    anim->hits.push_back(s);
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
    g_game.uanimations.push_back(anim);
    return true;
}

UCargoShip::UCargoShip(vec2i p)
    : UShip(UActorType::CargoShip, p)
{
    ship = generate("cargo", "cargo_ship");
    g_game.ships.push_back(ship);
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
    buffer.setTile(pos - origin, 'C', 0xFFFFFFFF, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x800000FF, LayerPriority_Overlay);
    }
}

UPirateShip::UPirateShip(vec2i p)
    : UShip(UActorType::PirateShip, p)
{
    ship = generate("pirate", "pirate_ship");
    g_game.ships.push_back(ship);
}

void UPirateShip::update(pcg32& rng)
{
    UShip::update(rng);

    if (target == 0)
    {
        // If no target, look for one every 10 turns
        if (check_for_target <= 0)
        {
            // Look for the closest visible ship within our sensor range
            float closest_distance = 999999;

            for (auto it : g_game.universe->actors)
            {
                if (it.value->type == UActorType::Player || it.value->type == UActorType::CargoShip)
                {
                    float dist = (pos - it.value->pos).length();
                    if (dist < closest_distance && dist < ship->sensor_range && g_game.universe->isVisible(pos, it.value->pos))
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
            float dist = (pos - s->pos).length();
            if (dist < ship->sensor_range && g_game.universe->isVisible(pos, s->pos))
            {
                // If the target is visible and within sensor range, update its last known position and potentially fire upon it
                target_last_pos = s->pos;
                if (dist < 12 && rng.nextFloat() < 0.6f)
                {
                    fireRailgun(s->pos);
                }
                else if (dist < 25 && rng.nextFloat() < 0.3f)
                {
                    fireTorpedo(s->pos);
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
    buffer.setTile(pos - origin, 'P', 0xFFFF0000, LayerPriority_Actors);
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
    buffer.setTile(pos - origin, '!', 0xFFFFFFFF, LayerPriority_Actors);
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
            buffer.setTile(pos + vec2i((int)round(cos(a) * r0), (int)round(sin(a) * r0)) - origin, '0', 0xFF505050, LayerPriority_Background-1);
        buffer.setTile(p - origin, getProjectileCharacter(rotate90(getDirection(a))), 0xFFFFFFFF, LayerPriority_Background);
    }
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
                if (t->source != a->id || (p.x == a->pos.x + d.x && p.y == a->pos.y + d.y))
                {
                    if (pl->ship)
                    {
                        float incoming = rng.nextFloat() * scalar::PIf * 2;
                        vec2f dir = vec2f(cos(incoming), sin(incoming));
                        pl->ship->explosion(dir, rng.nextFloat() * 20 + 10);
                    }
                    else
                    {
                        pl->dead = true;
                    }

                    if (a == g_game.uplayer)
                    {
                        g_game.log.log("Torpedo detonating on hull!");
                    }
                    else
                    {
                        if (t->source == g_game.uplayer->id)
                        {
                            if (t->target == a->id)
                                g_game.log.log("Target ship hit.");
                            else
                                g_game.log.log("Unknown ship hit.");
                        }
                    }
                    t->dead = true;
                }
            }
            else if (a->type == UActorType::Torpedo)
            {
                UTorpedo* at = (UTorpedo*) a;
                if (at->source != it.value->id || (p.x == a->pos.x + d.x && p.y == a->pos.y + d.y))
                {
                    if (isShipType(it.value->type) && ((UShip*) it.value)->ship)
                    {
                        float incoming = rng.nextFloat() * scalar::PIf * 2;
                        vec2f dir = vec2f(cos(incoming), sin(incoming));
                        UShip* os = (UShip*)it.value;
                        os->ship->explosion(dir, rng.nextFloat() * 20 + 10);
                    }

                    if (it.value == g_game.uplayer)
                    {
                        g_game.log.log("Torpedo detonating on hull!");
                    }
                    else
                    {
                        if (at->source == g_game.uplayer->id)
                        {
                            if (at->target == it.value->id)
                                g_game.log.log("Target ship hit.");
                            else
                                g_game.log.log("Torpedo detonated on unknown target.");
                        }
                    }
                    at->dead = true;
                    break;
                }
            }
            else if (pl->vel.length() > 6)
            {
                if (a == g_game.uplayer) g_game.log.log("Impact alert! Significant damage sustained.");

                vec2f dir = vec2f(rng.nextFloat() - 0.5f, 2.0f).normalize();
                pl->ship->explosion(dir, rng.nextFloat() * 8 * pl->vel.length() + 10);
                pl->vel = vec2i(0, 0);
                break;
            }
            else if (it.value->type == UActorType::Asteroid)
            {
                if (pl->vel.length() > 2)
                {
                    if (a == g_game.uplayer) g_game.log.log("Impact alert!");

                    vec2f dir = vec2f(rng.nextFloat() - 0.5f, 2.0f).normalize();
                    pl->ship->explosion(dir, rng.nextFloat() * 5 * pl->vel.length() + 5);
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
        last = p;
    }
    if (a->pos != last)
    {
        actors.erase(a->pos);
        a->pos = last;
        actors.insert(a->pos, a);
    }
}

void Universe::spawn(UActor* a)
{
    vec2i p = a->pos;
    int i = 0;
    int x = 0;
    int y = 0;
    while (actors.find(p).found)
    {
        p = a->pos + getOffset(i, x, y);
    }
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
                        if (rng.nextFloat() < 0.05f)
                        {
                            UAsteroid* a = new UAsteroid(vec2i((rx << 5) + (x0 << 3), (ry << 5) + (y0 << 3)));
                            a->sfreq = rng.nextFloat() * 10;
                            a->radius = 1.0f + rng.nextFloat() * 2.0f + rng.nextFloat() * rng.nextFloat() * 8.0f;
                            spawn(a);
                        }
                        else
                        {
                            if (rng.nextFloat() < 0.01f)
                            {
                                UCargoShip* s = new UCargoShip(vec2i((rx << 5) + (x0 << 4) + 2, (ry << 5) + (y0 << 3)));
                                spawn(s);
                            }
                            else if (rng.nextFloat() < 0.01f)
                            {
                                int count = 1 + rng.nextFloat() * rng.nextFloat() > 0.5f ? 1 : 0;
                                UPirateShip* s = new UPirateShip(vec2i((rx << 5) + (x0 << 4) + 2, (ry << 5) + (y0 << 3)));
                                spawn(s);
                            }
                        }
                    }
                }

                regions_generated.insert(vec2i(rx, ry), true);
            }
        }
    }

    std::vector<UShip*> moved;
    std::vector<UActor*> to_remove;
    for (auto it : actors)
    {
        UActor* a = it.value;
        if (a->pos != it.key) continue; // Proxy actor
        if ((a->pos - origin).length() > 160.0f)
        {
            to_remove.push_back(a);
            continue;
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
        actors.erase(a->pos);
        actor_ids.erase(a->id);
        if (a->type == UActorType::Asteroid)
        {
            // Remove proxies
            UAsteroid* ast = (UAsteroid*)a;
            int r2 = scalar::ceili(ast->radius);
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
        delete a;
    }
}

void Universe::render(TextBuffer& buffer, vec2i origin)
{
    vec2i bl = origin - vec2i(25, 22);
    for (auto it : actors)
    {
        if (it.value->pos == it.key)
            it.value->render(buffer, bl);
        // else
        //    buffer.setBg(it.key - bl, 0xFF303030, LayerPriority_Background);
    }
}
