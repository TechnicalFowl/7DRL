#pragma once

#include "util/linear_map.h"
#include "util/string.h"
#include "util/vector_math.h"

#include "types.h"

struct Actor;
struct Player;

struct Tile
{
    vec2i pos;
    Terrain terrain = Terrain::Empty;
    Actor* ground = nullptr;
    Actor* actor = nullptr;

    Tile(vec2i pos) : pos(pos) {}
    Tile(vec2i pos, Terrain trr) : pos(pos), terrain(trr) {}
};

struct Map
{
    sstring name;
    linear_map<vec2i, Tile> tiles;
    std::vector<Actor*> actors;
    vec2i min, max;

    Player* player;

    int turn = 0;

    Map(const sstring& name);

    void render(TextBuffer& buffer, vec2i origin);

    bool isPassable(vec2i p) const;
    Terrain getTile(vec2i p) const;
    void setTile(vec2i pos, Terrain trr);
    void trySetTile(vec2i pos, Terrain trr);

    bool spawn(Actor* a);
    bool move(Actor* a, vec2i to);

    void clear();

    vec2i findNearestEmpty(vec2i p, int max = 3);
    vec2i findNearestEmpty(vec2i p, Terrain trr, int max = 3);
};
