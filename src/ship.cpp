#include "ship.h"

#include "actor.h"
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
        case ActorType::Reactor: reactor = (Reactor*)a; break;
        case ActorType::Engine: engines.push_back((MainEngine*)a); break;
        case ActorType::TorpedoLauncher: torpedoes.push_back((TorpedoLauncher*)a); break;
        case ActorType::PDC: pdcs.push_back((PDC*)a); break;
        case ActorType::Railgun: railguns.push_back((Railgun*)a); break;
        default: break;
        }
    }

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
