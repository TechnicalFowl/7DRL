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
    bool explored = false;

    Tile(vec2i pos) : pos(pos) {}
    Tile(vec2i pos, Terrain trr) : pos(pos), terrain(trr) {}
};

struct Map
{
    sstring name;
    linear_map<vec2i, Tile> tiles;
    std::vector<Actor*> actors;
    vec2i min, max;

    Player* player = nullptr;
    bool see_all = true;

    int turn = 0;

    Map(const sstring& name);

    void render(TextBuffer& buffer, vec2i origin);

    bool isPassable(vec2i p) const;
    bool isExplored(vec2i p) const;
    Terrain getTile(vec2i p) const;
    void setTile(vec2i pos, Terrain trr);
    bool trySetTile(vec2i pos, Terrain trr);

    bool spawn(Actor* a);
    bool move(Actor* a, vec2i to);

    void clear();

    vec2i findNearestEmpty(vec2i p, int max = 3);
    vec2i findNearestEmpty(vec2i p, Terrain trr, int max = 3);

    std::vector<vec2i> findPath(vec2i from, vec2i to);
    std::vector<vec2i> findRay(vec2i from, vec2i to);

    bool isVisible(vec2i from, vec2i to);
};

struct ReferenceFrame
{
    Map& map;
    vec2i origin;
    Direction up;

    ReferenceFrame(Map& map, vec2i origin)
        : map(map), origin(origin), up(Up)
    {
    }

    ReferenceFrame(Map& map, vec2i origin, Direction up)
        : map(map), origin(origin), up(up)
    {
    }

    vec2i toLocal(vec2i p) const;
    vec2i toGlobal(vec2i p) const;

    bool isPassable(vec2i p) const { return map.isPassable(toGlobal(p)); }
    bool isExplored(vec2i p) const { return map.isExplored(toGlobal(p)); }
    Terrain getTile(vec2i p) const { return map.getTile(toGlobal(p)); }
    void setTile(vec2i pos, Terrain trr) { map.setTile(toGlobal(pos), trr); }
    bool trySetTile(vec2i pos, Terrain trr) { return map.trySetTile(toGlobal(pos), trr); }

    bool spawn(Actor* a) { return map.spawn(a); }
    bool move(Actor* a, vec2i to) { return map.move(a, toGlobal(to)); }

    vec2i findNearestEmpty(vec2i p, int max = 3) { return map.findNearestEmpty(toGlobal(p), max); }
    vec2i findNearestEmpty(vec2i p, Terrain trr, int max = 3) { return map.findNearestEmpty(toGlobal(p), trr, max); }

    bool isVisible(vec2i from, vec2i to) { return map.isVisible(toGlobal(from), toGlobal(to)); }
};
