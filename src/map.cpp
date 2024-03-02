#include "map.h"

#include <deque>

#include "actor.h"
#include "game.h"

Map::Map(const sstring& name)
    : name(name)
{
}

void Map::render(TextBuffer& buffer, vec2i origin)
{
    vec2i bl = origin - vec2i(25, 22);

    linear_map<vec2i, bool> los;
    for (int x = -10; x <= 10; ++x)
    {
        auto path = findRay(player->pos, player->pos + vec2i(x, 10));
        for (vec2i p: path) los.insert(p, true);
        path = findRay(player->pos, player->pos + vec2i(x, -10));
        for (vec2i p: path) los.insert(p, true);
    }
    for (int y = -9; y <= 9; ++y)
    {
        auto path = findRay(player->pos, player->pos + vec2i(10, y));
        for (vec2i p: path) los.insert(p, true);
        path = findRay(player->pos, player->pos + vec2i(-10, y));
        for (vec2i p: path) los.insert(p, true);
    }

    for (auto it : tiles)
    {
        bool visible = los.find(it.key).found;
        if (!it.value.explored)
        {
            if (!visible) continue;
            it.value.explored = true;
        }
        if (it.value.actor)
        {
            it.value.actor->render(buffer, bl, !visible);
        }
        else if (it.value.ground)
        {
            it.value.ground->render(buffer, bl, !visible);
        }
        if (it.value.terrain != Terrain::Empty)
        {
            TerrainInfo& ti = g_game.reg.terrain_info[(int)it.value.terrain];
            u32 bgcol = !visible ? scalar::convertToGrayscale(ti.bg_color, 0.5f) : ti.bg_color;
            buffer.setBg(it.value.pos - bl, bgcol, LayerPriority_Background);
            if (ti.color)
            {
                u32 col = !visible ? scalar::convertToGrayscale(ti.color, 0.5f) : ti.color;
                buffer.setTile(it.value.pos - bl, ti.character, col, LayerPriority_Tiles);
            }
        }
    }
}

bool Map::isPassable(vec2i p) const
{
    return g_game.reg.terrain_info[(int)getTile(p)].passable;
}

bool Map::isExplored(vec2i p) const
{
    auto it = tiles.find(p);
    if (it.found)
    {
        return it.value.explored;
    }
    return false;
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

std::vector<vec2i> Map::findPath(vec2i from, vec2i to)
{
    std::deque<vec2i> open;
    linear_map<vec2i, vec2i> came_from;
    linear_map<vec2i, int> cost_so_far;

    open.push_back(from);
    cost_so_far.insert(from, 0);

    while (!open.empty())
    {
        vec2i current = open.front();
        open.pop_front();
        if (current == to) break;
        for (vec2i d : cardinals)
        {
            vec2i next = current + d;
            if (!isPassable(next)) continue;
            int new_cost = cost_so_far[current] + 1;
            if (!cost_so_far.find(next).found || new_cost < cost_so_far[next])
            {
                cost_so_far.insert(next, new_cost);
                int priority = new_cost + abs(to.x - next.x) + abs(to.y - next.y);
                bool placed = false;
                for (auto it = open.begin(); it != open.end(); ++it)
                {
                    int it_priority = cost_so_far[*it] + abs(it->x - next.x) + abs(it->y - next.y);
                    if (it_priority > priority)
                    {
                        open.insert(it, next);
                        placed = true;
                        break;
                    }
                }
                if (!placed)
                    open.push_back(next);
                came_from.insert(next, current);
            }
        }
    }

    std::vector<vec2i> result;
    vec2i current = to;
    while (current != from)
    {
        result.push_back(current);
        auto it = came_from.find(current);
        if (!it.found) return std::vector<vec2i>();
        current = it.value;
    }
    result.push_back(from);
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<vec2i> Map::findRay(vec2i from, vec2i to)
{
    float x0 = from.x + 0.5f;
    float y0 = from.y + 0.5f;
    float x1 = to.x + 0.5f;
    float y1 = to.y + 0.5f;

    float dx = abs(x1 - x0);
    float dy = abs(y1 - y0);

    int x = scalar::floori(x0);
    int y = scalar::floori(y0);

    int n = 1 + scalar::floori(dx + dy);
    int x_inc = (x1 > x0) ? 1 : -1;
    int y_inc = (y1 > y0) ? 1 : -1;

    float error = dx - dy;
    dx *= 2;
    dy *= 2;

    std::vector<vec2i> result;
    for (; n > 0; --n)
    {
        result.push_back(vec2i(x, y));
        auto it = tiles.find(vec2i(x, y));
        if (it.found)
        {
            TerrainInfo& ti = g_game.reg.terrain_info[int(it.value.terrain)];
            if (!ti.passable) break;
            bool actor_passable = true;
            if (it.value.ground)
            {
                switch (it.value.ground->type)
                {
                case ActorType::InteriorDoor:
                {
                    InteriorDoor* door = (InteriorDoor*) it.value.ground;
                    actor_passable = door->open;
                } break;
                case ActorType::Airlock:
                {
                    Airlock* door = (Airlock*) it.value.ground;
                    actor_passable = door->open;
                } break;
                default: break;
                }
            }
            if (it.value.actor)
            {
                switch (it.value.actor->type)
                {
                case ActorType::Player:
                    break;
                default:
                {
                    actor_passable = false;
                } break;
                }
            }
            if (!actor_passable) break;
        }
        if (error > 0)
        {
            x += x_inc;
            error -= dy;
        }
        else
        {
            y += y_inc;
            error += dx;
        }
    }
    return result;
}

bool Map::isVisible(vec2i from, vec2i to)
{
    auto steps = findRay(from, to);
    return steps.back() == to;
}

vec2i ReferenceFrame::toLocal(vec2i p) const
{
    return rotateCCW(p - origin, up);
}

vec2i ReferenceFrame::toGlobal(vec2i p) const
{
    return rotateCW(p, up) + origin;
}
