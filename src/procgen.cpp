#include "procgen.h"

#include <deque>
#include <vector>

#include "util/random.h"

#include "map.h"

#if 0
typedef void (*room_builder)(Map& map, pcg32& rng, vec2i pos, vec2i size);

void empty_room_builder(Map& map, pcg32& rng, vec2i pos, vec2i size) {}

struct RoomPrototype
{
    struct Connection
    {
        vec2i min;
        vec2i max;
        Direction dir;
    };

    rect2i min_bounds;
    vec4i variance;
    std::vector<Connection> connectors;
    bool is_starting_room = false;
    room_builder builder;

    Terrain floor = Terrain::DirtFloor;
    Terrain walls = Terrain::StoneWall;

    RoomPrototype(vec2i lower, vec2i upper, vec4i variance, room_builder bldr, bool starting)
        : min_bounds(lower, upper), variance(variance), builder(bldr), is_starting_room(starting)
    {

    }
};

struct PrefabRoomGenerator
{
    struct PlacedRoom
    {
        struct Connector
        {
            vec2i pos;
            Direction dir;
        };

        vec2i origin;
        rect2i bounds;
        RoomPrototype* proto;
        std::vector<Connector> unused_connectors;

        PlacedRoom(vec2i o, rect2i bounds, RoomPrototype* proto) : bounds(bounds), proto(proto) {}
        PlacedRoom(vec2i o, vec2i min, vec2i max, RoomPrototype* proto) : bounds(min, max), proto(proto) {}
    };

    std::vector<RoomPrototype> rooms;
    int starting_room_count = 0;

    RoomPrototype& addRoom(vec2i lower, vec2i upper, vec4i variance, room_builder bldr, bool starting = false)
    {
        rooms.emplace_back(lower, upper, variance, bldr, starting);
        if (starting) starting_room_count++;
        return rooms.back();
    }

    PlacedRoom place(vec2i o, RoomPrototype* proto, pcg32& rng, vec4i allowed_variance)
    {
        rect2i bounds = proto->min_bounds;
        vec4i var;
        if(proto->variance.x > 0) var.x = rng.nextInt(0, scalar::min(proto->variance.x, allowed_variance.x));
        if(proto->variance.y > 0) var.y = rng.nextInt(0, scalar::min(proto->variance.y, allowed_variance.y));
        if(proto->variance.z > 0) var.z = rng.nextInt(0, scalar::min(proto->variance.z, allowed_variance.z));
        if(proto->variance.w > 0) var.w = rng.nextInt(0, scalar::min(proto->variance.w, allowed_variance.w));

        bounds.upper.y += var.x;
        bounds.upper.x += var.y;
        bounds.lower.y -= var.z;
        bounds.lower.x -= var.w;

        PlacedRoom placed(o, bounds, proto);

        for (auto& conn : proto->connectors)
        {
            vec2i pos(rng.nextInt(conn.min.x, conn.max.x), rng.nextInt(conn.min.y, conn.max.y));
            pos += direction(conn.dir) * var[conn.dir];
            placed.unused_connectors.emplace_back(pos, conn.dir);
        }
        return placed;
    }


    void generate(Map& map)
    {
        std::deque<PlacedRoom> stack;
        pcg32 rng;
        int start = rng.nextInt(0, starting_room_count);

        for (RoomPrototype& p : rooms)
        {
            if (p.is_starting_room && --start == 0)
            {
                stack.emplace_back(place(vec2i(), &p, rng, vec4i(10, 10, 10, 10)));
                break;
            }
        }
        debug_assert(!stack.empty());

        while (!stack.empty())
        {
            PlacedRoom& placed = stack.front();
            stack.pop_front();

            // Look for a prefab to attach to each connector
            for (PlacedRoom::Connector& conn : placed.unused_connectors)
            {
                vec2i pos = placed.bounds.lower + conn.pos;
                if (map.getTile(pos) != Terrain::StoneWall) continue;
                vec2i dir = direction(conn.dir);
                vec2i next = pos + dir;
                if (map.getTile(next) == Terrain::StoneWall)
                {
                    RoomPrototype* next_proto = nullptr;
                    for (RoomPrototype& p : rooms)
                    {
                        if (p.is_starting_room) continue;
                        if (p.min_bounds.contains(next))
                        {
                            next_proto = &p;
                            break;
                        }
                    }
                    if (next_proto)
                    {
                        PlacedRoom next_placed = place(conn.pos, next_proto, rng, vec4i(10, 10, 10, 10));

                        // @TODO: Place room into world
                        next_placed.proto->builder(map, rng, placed.bounds.lower, placed.bounds.upper - placed.bounds.lower + vec2i(1, 1));
                        
                        stack.push_back(next_placed);
                    }
                }
            }
        }
    }
};

struct DenseRoomGenerator
{
    struct Room
    {
        rect2i bounds;

        Room(rect2i b)
            : bounds(b)
        {
        }
        Room(vec2i min, vec2i max)
            : bounds(min, max)
        {
        }

        vec2i getWallStart(Direction dir)
        {
            switch (dir)
            {
            case Up: return vec2i(bounds.lower.x, bounds.upper.y);
            case Down: return vec2i(bounds.upper.x, bounds.lower.y);
            case Left: return vec2i(bounds.lower.x, bounds.lower.y);
            case Right: return vec2i(bounds.upper.x, bounds.upper.y);
            default: break;
            }
            debug_assert(false);
            return vec2i();
        }

    };


    int max_rooms = 100;
    pcg32 rng;

    void apply(ReferenceFrame& map, Room& room)
    {
        vec2i min0 = map.toLocal(room.bounds.lower);
        vec2i max0 = map.toLocal(room.bounds.upper);
        vec2i min = ::min(min0, max0);
        vec2i max = ::max(min0, max0);

        for (int y = min.y + 1; y < max.y; ++y)
        {
            for (int x = min.x + 1; x < max.x; ++x)
            {
                map.trySetTile(vec2i(x, y), Terrain::DirtFloor);
            }
        }
        bool nx_door = map.up == Right;
        bool px_door = map.up == Left;
        bool ny_door = map.up == Up;
        bool py_door = map.up == Down;
        for (int y = min.y; y <= max.y; ++y)
        {
            if (!px_door && map.getTile(vec2i(max.x + 1, y)) != Terrain::Empty && map.isPassable(vec2i(max.x + 1, y)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(max.x, y), Terrain::DirtFloor);
                px_door = true;
            }
            else
                map.trySetTile(vec2i(max.x, y), Terrain::StoneWall);
            if (!nx_door && map.getTile(vec2i(min.x - 1, y)) != Terrain::Empty && map.isPassable(vec2i(min.x - 1, y)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(min.x, y), Terrain::DirtFloor);
                nx_door = true;
            }
            else
                map.trySetTile(vec2i(min.x, y), Terrain::StoneWall);
        }
        for (int x = min.x; x <= max.x; ++x)
        {
            if (!ny_door && map.getTile(vec2i(x, min.y - 1)) != Terrain::Empty && map.isPassable(vec2i(x, min.y - 1)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(x, min.y), Terrain::DirtFloor);
                ny_door = true;
            }
            else
                map.trySetTile(vec2i(x, min.y), Terrain::StoneWall);
            if (!py_door && map.getTile(vec2i(x, max.y + 1)) != Terrain::Empty && map.isPassable(vec2i(x, max.y + 1)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(x, max.y), Terrain::DirtFloor);
                py_door = true;
            }
            else
                map.trySetTile(vec2i(x, max.y), Terrain::StoneWall);
        }
    }

    bool isEmpty(Map& map, vec2i p)
    {
        return !map.tiles.find(p).found;
    }

    void generate(Map& map)
    {
        std::deque<Room> stack;

        Room start(vec2i(rng.nextInt(-5, -2), rng.nextInt(-5, -2)), vec2i(rng.nextInt(2, 5), rng.nextInt(2, 5)));
        stack.push_back(start);
        {
            ReferenceFrame frame(map, vec2i(), Up);
            apply(frame, start);
        }

        float max_x = 40.0f;
        float max_y = 20.0f;

        int rooms = 1;

        while (!stack.empty())
        {
            Room room = stack.back();
            stack.pop_back();

            int x = room.bounds.lower.x;
            int y = room.bounds.lower.y;
            int w = room.bounds.upper.x - room.bounds.lower.x + 1;
            int h = room.bounds.upper.y - room.bounds.lower.y + 1;

            for (int i = 0; i < 4; ++i) {
                Direction dir = cardinal_dirs[i];
                vec2i begin_pos = room.getWallStart(dir);
                vec2i step_adj = direction(dir);
                vec2i step = direction(rotate90(dir));
                vec2i end_pos = room.getWallStart(rotate90(dir));
                int length = scalar::floori((end_pos - begin_pos).length());
                debug_assert(begin_pos + step * length == end_pos);

                vec2i top = (begin_pos + end_pos) / 2 + step_adj;
                if (top.length() / max_y < 1)
                {
                    ReferenceFrame frame(map, begin_pos + step_adj, dir);
                    int start = -1;
                    int end = -1;
                    for (int x0 = 1; x0 < length - 1; ++x0)
                    {
                        if (start == -1 && frame.getTile(vec2i(x0, 0)) == Terrain::Empty)
                        {
                            start = x0;
                        }
                        else if (start >= 0 && frame.getTile(vec2i(x0, 0)) != Terrain::Empty)
                        {
                            end = x0 - 1;
                            break;
                        }
                    }
                    if (start >= 0)
                    {
                        if (end < 0) end = length - 1;
                        if (end - start < 3) continue;
                        int door = rng.nextInt(start, end);
                        int nx = rng.nextInt(-8, -2);
                        int px = rng.nextInt(2, 8);
                        int py = rng.nextInt(3, 8);
                        int x0 = door;
                        for (; x0 > door + nx; --x0)
                            if (frame.getTile(vec2i(x0, 0)) != Terrain::Empty) break;
                        int x1 = door;
                        for (; x1 < door + px; ++x1)
                            if (frame.getTile(vec2i(x1, 0)) != Terrain::Empty) break;
                        if (x1 - x0 < 3) continue;
                        int y0 = 0;
                        bool found = false;
                        for (; y0 < py && !found; ++y0)
                        {
                            for (int x2 = x0 + 1; x2 < x1; ++x2)
                            {
                                if (frame.getTile(vec2i(x2, y0)) != Terrain::Empty)
                                {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if (y0 < 3) continue;
                        frame.setTile(vec2i(door, -1), Terrain::DirtFloor);
                        Room r(frame.toGlobal(vec2i(x0, -1)), frame.toGlobal(vec2i(x1, y0)));
                        stack.push_front(r);
                        apply(frame, r);
                        rooms++;
                        if (rooms > max_rooms) break;
                    }
                }
            }
        }
    }
};
#endif

void generate(Map& map)
{
    if (map.name == "level_0")
    {

    }
    else
    {
        debug_assert(false);
    }
}
