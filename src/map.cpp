#include "map.h"

#include "actor.h"
#include "game.h"

vec2i directions[]{
    vec2i(0, 1),
    vec2i(1, 0),
    vec2i(0, -1),
    vec2i(-1, 0)
};

vec2i direction(Direction d) { return directions[d]; }

Direction rotate_90_cw[]{
    Right,
    Down,
    Left,
    Up,
};
Direction rotate_180[]{
    Down,
    Left,
    Up,
    Right,
};
Direction rotate_270_cw[]{
    Left,
    Up,
    Right,
    Down,
};

vec2i rotateCW(vec2i p, Direction dir)
{
    switch (dir)
    {
    case Left:
        return vec2i(-p.y, p.x);
    case Down:
        return vec2i(-p.x, -p.y);
    case Right:
        return vec2i(p.y, -p.x);
    default:
    case Up:
        return p;
    }
}

vec2i rotateCCW(vec2i p, Direction dir)
{
    switch (dir)
    {
    case Right:
        return vec2i(-p.y, p.x);
    case Down:
        return vec2i(-p.x, -p.y);
    case Left:
        return vec2i(p.y, -p.x);
    default:
    case Up:
        return p;
    }
}

Map::Map(const sstring& name)
    : name(name)
{
}

void Map::render(TextBuffer& buffer, vec2i origin)
{
    vec2i bl = origin - vec2i(25, 22);

    for (auto it : tiles)
    {
        if (it.value.actor)
        {
            it.value.actor->render(buffer, bl);
        }
        else if (it.value.ground)
        {
            it.value.ground->render(buffer, bl);
        }
        if (it.value.terrain != Terrain::Empty)
        {
            TerrainInfo& ti = g_game.reg.terrain_info[(int)it.value.terrain];
            buffer.setBg(it.value.pos - bl, ti.bg_color, LayerPriority_Background);
            buffer.setTile(it.value.pos - bl, ti.character, ti.color, LayerPriority_Tiles);
        }
    }
}

bool Map::isPassable(vec2i p) const
{
    return g_game.reg.terrain_info[(int)getTile(p)].passable;
}

Terrain Map::getTile(vec2i p) const
{
    auto it = tiles.find(p);
    if (it.found)
    {
        return it.value.terrain;
    }
    return Terrain::Empty;
}

void Map::setTile(vec2i pos, Terrain trr)
{
    auto it = tiles.find(pos);
    if (it.found)
    {
        it.value.terrain = trr;
        if (it.value.actor)
        {
            // @Todo: Move to nearest empty space
        }
    }
    else
    {
        tiles.insert(pos, Tile(pos, trr));
        min = ::min(min, pos);
        max = ::max(max, pos);
    }
}

bool Map::trySetTile(vec2i pos, Terrain trr)
{
    auto it = tiles.find(pos);
    if (it.found)
        return false;
    tiles.insert(pos, Tile(pos, trr));
    min = ::min(min, pos);
    max = ::max(max, pos);
    return true;
}

bool Map::spawn(Actor* a)
{
    ActorInfo& ai = g_game.reg.actor_info[int(a->type)];
    actors.push_back(a);
    auto it = tiles.find(a->pos);
    if (it.found)
    {
        TerrainInfo& ti = g_game.reg.terrain_info[(int)it.value.terrain];
        if (!ti.passable || (ai.is_ground && it.value.ground) || (!ai.is_ground && it.value.actor)) return false;
        if (ai.is_ground) it.value.ground = a;
        else it.value.actor = a;
    }
    else
    {
        Tile tl(a->pos);
        if (ai.is_ground) tl.ground = a;
        else tl.actor = a;
        tiles.insert(a->pos, tl);
        min = ::min(min, a->pos);
        max = ::max(max, a->pos);
    }
    return true;
}

bool Map::move(Actor* a, vec2i to)
{
    ActorInfo& ai = g_game.reg.actor_info[int(a->type)];
    auto it = tiles.find(a->pos);
    debug_assert(it.found);
    auto it2 = tiles.find(to);
    if (it2.found)
    {
        TerrainInfo& ti = g_game.reg.terrain_info[(int)it.value.terrain];
        if (!ti.passable || (ai.is_ground && it2.value.ground) || (!ai.is_ground && it2.value.actor)) return false;
        if (ai.is_ground)
        {
            it.value.ground = nullptr;
            it2.value.ground = a;
        }
        else
        {
            it.value.actor = nullptr;
            it2.value.actor = a;
        }
    }
    else
    {
        Tile tl(to);
        if (ai.is_ground)
        {
            it.value.ground = nullptr;
            tl.ground = a;
        }
        else
        {
            it.value.actor = nullptr;
            tl.actor = a;
        }
        tiles.insert(to, tl);
        min = ::min(min, to);
        max = ::max(max, to);
    }
    a->pos = to;
    return true;
}

void Map::clear()
{
    tiles.clear();
    actors.clear();
    turn = 0;
    min = vec2i(INT32_MAX, INT32_MAX);
    max = vec2i(INT32_MIN, INT32_MIN);
}

vec2i Map::findNearestEmpty(vec2i p, int max)
{
    for (int i = 1; i <= max; ++i)
    {
        for (int x = p.x - i; x <= p.x + i; ++x)
        {
            if (isPassable(vec2i(x, p.y + i))) return vec2i(x, p.y + i);
            if (isPassable(vec2i(x, p.y - i))) return vec2i(x, p.y - i);
        }
        for (int y = p.y - i + 1; y < p.y + i; ++y)
        {
            if (isPassable(vec2i(p.x + i, y))) return vec2i(p.x + i, y);
            if (isPassable(vec2i(p.x - i, y))) return vec2i(p.x - i, y);
        }
    }
    return p; // No nearaby place found!
}

vec2i Map::findNearestEmpty(vec2i p, Terrain trr, int max)
{
    for (int i = 1; i <= max; ++i)
    {
        for (int x = p.x - i; x <= p.x + i; ++x)
        {
            if (getTile(vec2i(x, p.y + i)) == trr) return vec2i(x, p.y + i);
            if (getTile(vec2i(x, p.y - i)) == trr) return vec2i(x, p.y - i);
        }
        for (int y = p.y - i + 1; y < p.y + i; ++y)
        {
            if (getTile(vec2i(p.x + i, y)) == trr) return vec2i(p.x + i, y);
            if (getTile(vec2i(p.x - i, y)) == trr) return vec2i(p.x - i, y);
        }
    }
    return p; // No nearaby place found!
}

vec2i ReferenceFrame::toLocal(vec2i p) const
{
    return rotateCCW(p - origin, up);
}

vec2i ReferenceFrame::toGlobal(vec2i p) const
{
    return rotateCW(p, up) + origin;
}
