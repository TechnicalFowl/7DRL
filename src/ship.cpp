#include "ship.h"

#include "actor.h"
#include "game.h"
#include "map.h"

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

std::vector<ShipRoom*> Ship::getRooms(RoomType t)
{
    std::vector<ShipRoom*> result;
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
        case ActorType::Scanner: scanner = (Scanner*) a; break;
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
                g_game.log.log("Your reactor is overloaded and will shutdown!");
            reactor->status = ShipObject::Status::Disabled;
            for (ShipObject* o : all_objects)
                if (o->status == ShipObject::Status::Active)
                    o->status = ShipObject::Status::Unpowered;
        }
    }
    else
    {
        map->see_all = false;
        for (ShipObject* o : all_objects)
            if (o->status == ShipObject::Status::Active)
                o->status = ShipObject::Status::Unpowered;
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
            hull_integrity--;
            auto it = map->tiles.find(p + vec2i(x, y));
            if (it.found)
            {
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

void Ship::railgun(vec2i d)
{
    if (!map)
    {
        hull_integrity = 0;
        return;
    }

}

float Ship::scannerRange() const
{
    if (!scanner) return 1;
    if (scanner->status != ShipObject::Status::Active) return 1;
    return scanner->range;
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
