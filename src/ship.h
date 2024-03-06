#pragma once

#include <vector>

#include "util/vector_math.h"

struct Actor;
struct Map;

struct MainEngine;
struct Reactor;
struct PilotSeat;
struct TorpedoLauncher;
struct PDC;
struct Railgun;

enum class RoomType
{
    Engine,
    Reactor,
    PilotsDeck,
    OperationsDeck,
    StorageRoom,
    Coridor,
    Airlock,
    TorpedoLauncher,
    PDC,
    Railgun,
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

    std::vector<MainEngine*> engines;
    Reactor* reactor = nullptr;
    PilotSeat* pilot = nullptr;
    std::vector<TorpedoLauncher*> torpedoes;
    std::vector<PDC*> pdcs;
    std::vector<Railgun*> railguns;

    int hull_integrity = 500;
    float sensor_range = 35;

    Ship(Map* map) : map(map) {}

    ShipRoom* getRoom(vec2i p);
    ShipRoom* getRoom(RoomType t);

    std::vector<ShipRoom*> getRooms(RoomType t);

    void update();

    void explosion(vec2f d, float power);
    void explosionAt(vec2i p, float power);
    void railgun(vec2i d);
};

std::vector<Actor*> findDoors(Ship* ship, vec2i p);