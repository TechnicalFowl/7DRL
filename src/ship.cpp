#include "ship.h"

#include "actor.h"
#include "game.h"
#include "map.h"
#include "sound.h"
#include "universe.h"

ShipRoom* Ship::getRoom(vec2i p)
{
    for (auto& r : rooms)
    {
        if (p.x >= r.min.x && p.x <= r.max.x && p.y >= r.min.y && p.y <= r.max.y)
            return &r;
    }
    return nullptr;
}

ShipRoom* Ship::getRoom(RoomType t)
{
    for (auto& r : rooms)
        if (r.type == t)
            return &r;
    return nullptr;
}

std::vector<const ShipRoom*> Ship::getRooms(RoomType t) const
{
    std::vector<const ShipRoom*> result;
    for (auto& r : rooms)
        if (r.type == t)
            result.push_back(&r);
    return result;
}

void Ship::update()
{
    engines.clear();
    pdcs.clear();
    torpedoes.clear();
    railguns.clear();
    for (Actor* a : map->actors)
    {
        switch (a->type)
        {
        case ActorType::PilotSeat: pilot = (PilotSeat*)a; break;
        case ActorType::Scanner: scanner = (Scanner*)a; break;
        case ActorType::Reactor: reactor = (Reactor*)a; break;
        case ActorType::Engine: engines.push_back((MainEngine*)a); break;
        case ActorType::TorpedoLauncher: torpedoes.push_back((TorpedoLauncher*)a); break;
        case ActorType::PDC: pdcs.push_back((PDC*)a); break;
        case ActorType::Railgun: railguns.push_back((Railgun*)a); break;
        default: break;
        }
    }
    std::vector<ShipObject*> all_objects;
    all_objects.insert(all_objects.end(), engines.begin(), engines.end());
    if (pilot) all_objects.push_back(pilot);
    if (scanner) all_objects.push_back(scanner);
    all_objects.insert(all_objects.end(), pdcs.begin(), pdcs.end());
    all_objects.insert(all_objects.end(), torpedoes.begin(), torpedoes.end());
    all_objects.insert(all_objects.end(), railguns.begin(), railguns.end());

    if (reactor && reactor->status == ShipObject::Status::Active)
    {
        map->see_all = true;
        float power_used = 0.0f;
        for (ShipObject* o : all_objects)
        {
            if (o->status == ShipObject::Status::Active)
                power_used += o->power_required;
            else if (o->status == ShipObject::Status::Unpowered)
                o->status = ShipObject::Status::Disabled;
        }

        reactor->power = power_used;
        if (reactor->power > reactor->capacity)
        {
            if (this == g_game.player_ship)
            {
                g_game.log.log("Your reactor is overloaded, shutting down non-essential systems!");
                playSound(SoundEffect::ReactorShutdown);
            }

            while (reactor->power > reactor->capacity)
            {
                bool shutdown_something = false;
                for (Railgun* r : railguns)
                {
                    if (r->status == ShipObject::Status::Active)
                    {
                        reactor->power -= r->power_required;
                        r->status = ShipObject::Status::Unpowered;
                        shutdown_something = true;
                        break;
                    }
                }
                if (shutdown_something) continue;
                for (TorpedoLauncher* r : torpedoes)
                {
                    if (r->status == ShipObject::Status::Active)
                    {
                        reactor->power -= r->power_required;
                        r->status = ShipObject::Status::Unpowered;
                        shutdown_something = true;
                        break;
                    }
                }
                if (shutdown_something) continue;
                for (MainEngine* r : engines)
                {
                    if (r->status == ShipObject::Status::Active)
                    {
                        reactor->power -= r->power_required;
                        r->status = ShipObject::Status::Unpowered;
                        shutdown_something = true;
                        break;
                    }
                }
                if (!shutdown_something) break;
            }

            if (reactor->power > reactor->capacity)
            {
                if (this == g_game.player_ship)
                    g_game.log.log("Your reactor is still overloaded! Power systems failing");
                reactor->power = 0;
                reactor->status = ShipObject::Status::Disabled;
                for (ShipObject* o : all_objects)
                {
                    if (o->status == ShipObject::Status::Active)
                        o->status = ShipObject::Status::Unpowered;
                }
            }
        }
    }
    else
    {
        map->see_all = false;
        for (ShipObject* o : all_objects)
            if (o->status == ShipObject::Status::Active)
                o->status = ShipObject::Status::Unpowered;
    }
    if (scanner && scanner->status == ShipObject::Status::Disabled)
    {
        scanner->status = ShipObject::Status::Active;
    }

    for (TorpedoLauncher* t : torpedoes)
    {
        if (t->status != ShipObject::Status::Active)
            t->charge_time = 2;
        else if (t->charge_time > 0)
            t->charge_time--;
    }
    for (Railgun* t : railguns)
    {
        if (t->status != ShipObject::Status::Active)
            t->charge_time = 2;
        else if (t->charge_time > 0)
            t->charge_time--;
    }
}

void Ship::explosion(vec2f d, float power)
{
    if (!map)
    {
        hull_integrity = 0;
        return;
    }
    vec2i center = (map->max + map->min) / 2;
    std::vector<vec2i> ray = map->findRay(center + (d * (map->max - center).length()).cast<int>(), center);
    vec2i p = ray.back();
    explosionAt(p, power);
}

void Ship::damageTile(vec2i p)
{
    auto it = map->tiles.find(p);
    if (it.found)
    {
        hull_integrity--;
        if (this == g_game.player_ship) g_game.uplayer->damage_taken++;
        if (it.value.terrain == Terrain::ShipWall)
            it.value.terrain = Terrain::DamagedShipWall;
        else if (it.value.terrain == Terrain::ShipFloor)
            it.value.terrain = Terrain::DamagedShipFloor;
        if (it.value.actor)
        {
            switch (it.value.actor->type)
            {
            case ActorType::PilotSeat:
            case ActorType::Reactor:
            case ActorType::Engine:
            case ActorType::TorpedoLauncher:
            case ActorType::PDC:
            case ActorType::Railgun:
            {
                ShipObject* obj = (ShipObject*)it.value.actor;
                obj->status = ShipObject::Status::Damaged;
            } break;
            }
        }
    }
}

void Ship::explosionAt(vec2i p, float power)
{
    if (!map)
    {
        hull_integrity = 0;
        return;
    }
    int r = scalar::ceili(power);
    for (int y = -r; y <= r; ++y)
    {
        for (int x = -r; x <= r; ++x)
        {
            int d2 = x * x + y * y;
            if (d2 > r * r) continue;
            float chance = 1 - sqrtf(x * x + y * y) / power;
            chance *= chance;
            if (g_game.rng.nextFloat() > chance) continue;
            damageTile(p + vec2i(x, y));
        }
    }

    if (this == g_game.player_ship)
    {
        for (int i = 0; i < 3; ++i)
        {
            vec2i c = p + vec2i(g_game.rng.nextInt(-3, 3), g_game.rng.nextInt(-3, 3));
            ExplosionAnimation* e = new ExplosionAnimation(c, scalar::ceili(power + g_game.rng.nextFloat() * 3 - 1.5f));
            g_game.animations.push_back(e);
        }
    }
}

void Ship::railgun(vec2i d, int power)
{
    if (!map)
    {
        hull_integrity = 0;
        return;
    }
    vec2i f, t;
    vec2i a;
    if (d.x == 0)
    {
        a = vec2i(1, 0);
        f = vec2i(map->min.x, g_game.rng.nextInt(map->min.y, map->max.y));
        t = vec2i(map->max.x, g_game.rng.nextInt(map->min.y, map->max.y));
    }
    else if (d.y == 0)
    {
        a = vec2i(0, 1);
        f = vec2i(g_game.rng.nextInt(map->min.x, map->max.x), map->min.y);
        t = vec2i(g_game.rng.nextInt(map->min.x, map->max.x), map->max.y);
    }
    else if (d.x < 0 != d.y < 0)
    {
        a = vec2i(1, 1);
        int hx = (map->max.x + map->min.x) / 2;
        int hy = (map->max.y + map->min.y) / 2;
        if (g_game.rng.nextBool())
        {
            f = vec2i(map->min.x, g_game.rng.nextInt(hy, map->max.y));
            t = vec2i(map->max.x, g_game.rng.nextInt(map->min.y, hy));
        }
        else
        {
            f = vec2i(g_game.rng.nextInt(map->min.x, hx), map->max.y);
            t = vec2i(g_game.rng.nextInt(hx, map->max.x), map->min.y);
        }
    }
    else
    {
        a = vec2i(-1, 1);
        int hx = (map->max.x + map->min.x) / 2;
        int hy = (map->max.y + map->min.y) / 2;
        if (g_game.rng.nextBool())
        {
            f = vec2i(map->min.x, g_game.rng.nextInt(map->min.y, hy));
            t = vec2i(map->max.x, g_game.rng.nextInt(hy, map->max.y));
        }
        else
        {
            f = vec2i(g_game.rng.nextInt(hx, map->max.x), map->max.y);
            t = vec2i(g_game.rng.nextInt(map->min.x, hx), map->min.y);
        }
    }

    auto points = findRay(f, t);
    for (vec2i p : points)
    {
        damageTile(p);
        for (int j = 1; j < power; ++j)
        {
            damageTile(p + a * j);
            damageTile(p + a * -j);
        }
    }
}

float Ship::scannerRange() const
{
    if (!scanner) return 1;
    if (scanner->status != ShipObject::Status::Active) return 1;
    return scanner->range;
}

void Ship::repair(int points)
{
    hull_integrity = scalar::min(hull_integrity + points, max_integrity);
    if (map)
    {
        int remaining = points;
        for (auto it : map->tiles)
        {
            if (it.value.terrain == Terrain::DamagedShipFloor)
            {
                remaining--;
                it.value.terrain = Terrain::ShipFloor;
                if (remaining <= 0) break;
            } else if (it.value.terrain == Terrain::DamagedShipWall)
            {
                remaining--;
                it.value.terrain = Terrain::ShipWall;
                if (remaining <= 0) break;
            }
        }
    }
}

bool Ship::airlockFull() const
{
    if (!map) return true;
    auto airlocks = getRooms(RoomType::Airlock);
    for (const ShipRoom* r : airlocks)
    {
        for (int y = r->min.y; y <= r->max.y; ++y)
        {
            for (int x = r->min.x; x <= r->max.x; ++x)
            {
                auto it = map->tiles.find(vec2i(x, y));
                if (it.found)
                {
                    if ((it.value.terrain == Terrain::ShipFloor || it.value.terrain == Terrain::DamagedShipFloor) && !it.value.ground && !it.value.actor)
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Ship::spawnItemInAirlock(Item* item)
{
    if (!map) return false;
    auto airlocks = getRooms(RoomType::Airlock);
    for (const ShipRoom* r : airlocks)
    {
        for (int y = r->min.y; y <= r->max.y; ++y)
        {
            for (int x = r->min.x; x <= r->max.x; ++x)
            {
                auto it = map->tiles.find(vec2i(x, y));
                if (it.found)
                {
                    if ((it.value.terrain == Terrain::ShipFloor || it.value.terrain == Terrain::DamagedShipFloor) && !it.value.ground && !it.value.actor)
                    {
                        GroundItem* gr = new GroundItem(vec2i(x, y), item);
                        map->spawn(gr);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

std::vector<Actor*> findDoors(Ship* ship, vec2i p)
{
    std::vector<Actor*> doors;
    ShipRoom* room = ship->getRoom(p);
    if (!room) return doors;

    for (int x = room->min.x; x <= room->max.x; ++x)
    {
        auto it = ship->map->tiles.find(vec2i(x, room->min.y));
        if (it.found && it.value.ground)
            if (it.value.ground->type == ActorType::InteriorDoor || it.value.ground->type == ActorType::Airlock)
                doors.push_back(it.value.ground);
        auto it2 = ship->map->tiles.find(vec2i(x, room->max.y));
        if (it2.found && it2.value.ground)
            if (it2.value.ground->type == ActorType::InteriorDoor || it2.value.ground->type == ActorType::Airlock)
                doors.push_back(it2.value.ground);
    }
    for (int y = room->min.y + 1; y < room->max.y; ++y)
    {
        auto it = ship->map->tiles.find(vec2i(room->min.x, y));
        if (it.found && it.value.ground)
            if (it.value.ground->type == ActorType::InteriorDoor || it.value.ground->type == ActorType::Airlock)
                doors.push_back(it.value.ground);
        auto it2 = ship->map->tiles.find(vec2i(room->max.x, y));
        if (it2.found && it2.value.ground)
            if (it2.value.ground->type == ActorType::InteriorDoor || it2.value.ground->type == ActorType::Airlock)
                doors.push_back(it2.value.ground);
    }

    return doors;
}
