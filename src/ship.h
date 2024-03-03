#pragma once

#include <vector>

#include "util/vector_math.h"

struct Actor;
struct Map;

enum class RoomType
{
    Engine,
    Reactor,
    PilotsDeck,
    OperationsDeck,
    StorageRoom,
    Coridor,
    Airlock,
};

struct ShipRoom
{
    vec2i min, max;
    RoomType type;

    ShipRoom(vec2i min, vec2i max, RoomType type) : min(min), max(max), type(type) {}
};

struct Ship
{
    Map* map;
    std::vector<ShipRoom> rooms;

    Ship(Map* map) : map(map) {}

    ShipRoom* getRoom(vec2i p);
    ShipRoom* getRoom(RoomType t);
};

std::vector<Actor*> findDoors(Ship* ship, vec2i p);