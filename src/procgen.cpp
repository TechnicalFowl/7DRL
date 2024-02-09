#include "procgen.h"

#include <deque>
#include <vector>

#include "util/random.h"

#include "map.h"

enum Direction
{
    Up,
    Right,
    Down,
    Left,
};

vec2i directions[] {
    vec2i(0, 1),
    vec2i(1, 0),
    vec2i(0, -1),
    vec2i(-1, 0)
};

Direction rotate_90[]{
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
Direction rotate_270[]{
    Left,
    Up,
    Right,
    Down,
};

vec2i rotate(vec2i p, Direction dir)
{
    while (dir != Up)
    {
        dir = rotate_90[dir];
        p = vec2i(p.y, -p.x);
    }
    return p;
}

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
            pos += directions[conn.dir] * var[conn.dir];
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
                vec2i dir = directions[conn.dir];
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
    };


    int max_rooms = 100;
    pcg32 rng;

    void apply(Map& map, Room& room, int door_dir)
    {
        for (int y = room.bounds.lower.y + 1; y < room.bounds.upper.y; ++y)
        {
            for (int x = room.bounds.lower.x + 1; x < room.bounds.upper.x; ++x)
            {
                map.trySetTile(vec2i(x, y), Terrain::DirtFloor);
            }
        }
        bool nx_door = door_dir == 3;
        bool px_door = door_dir == 2;
        bool ny_door = door_dir == 1;
        bool py_door = door_dir == 0;
        for (int y = room.bounds.lower.y; y <= room.bounds.upper.y; ++y)
        {
            if (!px_door && !isEmpty(map, vec2i(room.bounds.upper.x + 1, y)) && map.isPassable(vec2i(room.bounds.upper.x + 1, y)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(room.bounds.upper.x, y), Terrain::DirtFloor);
                px_door = true;
            }
            else
                map.trySetTile(vec2i(room.bounds.upper.x, y), Terrain::StoneWall);
            if (!nx_door && !isEmpty(map, vec2i(room.bounds.lower.x - 1, y)) && map.isPassable(vec2i(room.bounds.lower.x - 1, y)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(room.bounds.lower.x, y), Terrain::DirtFloor);
                nx_door = true;
            }
            else
                map.trySetTile(vec2i(room.bounds.lower.x, y), Terrain::StoneWall);
        }
        for (int x = room.bounds.lower.x; x <= room.bounds.upper.x; ++x)
        {
            if (!ny_door && !isEmpty(map, vec2i(x, room.bounds.lower.y - 1)) && map.isPassable(vec2i(x, room.bounds.lower.y - 1)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(x, room.bounds.lower.y), Terrain::DirtFloor);
                ny_door = true;
            }
            else
                map.trySetTile(vec2i(x, room.bounds.lower.y), Terrain::StoneWall);
            if (!py_door && !isEmpty(map, vec2i(x, room.bounds.upper.y + 1)) && map.isPassable(vec2i(x, room.bounds.upper.y + 1)) && rng.nextFloat() < 0.2f)
            {
                map.trySetTile(vec2i(x, room.bounds.upper.y), Terrain::DirtFloor);
                py_door = true;
            }
            else
                map.trySetTile(vec2i(x, room.bounds.upper.y), Terrain::StoneWall);
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
        apply(map, start, -1);

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

            vec2i top(x + w / 2, y + h);
            if (top.length() / max_y < 1)
            {
                int start = x - 1;
                int end = x - 1;
                for (int x0 = x + 1; x0 < x + w - 1; ++x0)
                {
                    if (start < x && isEmpty(map, vec2i(x0, y + h)))
                    {
                        start = x0;
                    }
                    else if (start >= x && !isEmpty(map, vec2i(x0, y + h)))
                    {
                        end = x0 - 1;
                        break;
                    }
                }
                if (start >= x)
                {
                    if (end < x) end = x + w - 1;
                    if (end - start < 3) goto bottom_room;
                    int door = rng.nextInt(start, end);
                    int nx = rng.nextInt(-8, -2);
                    int px = rng.nextInt(2, 8);
                    int py = rng.nextInt(3, 8);
                    int x0 = door;
                    for (; x0 > door + nx; --x0)
                        if (!isEmpty(map, vec2i(x0, y + h))) break;
                    int x1 = door;
                    for (; x1 < door + px; ++x1)
                        if (!isEmpty(map, vec2i(x1, y + h))) break;
                    if (x1 - x0 < 3) goto bottom_room;
                    int y0 = y + h;
                    bool found = false;
                    for (; y0 < y + h + py && !found; ++y0)
                    {
                        for (int x2 = x0 + 1; x2 < x1; ++x2)
                        {
                            if (!isEmpty(map, vec2i(x2, y0)))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (y0 - (y + h) < 3) goto bottom_room;
                    map.setTile(vec2i(door, y + h - 1), Terrain::DirtFloor);
                    Room r(vec2i(x0, y + h - 1), vec2i(x1, y0));
                    stack.push_front(r);
                    apply(map, r, 1);
                    rooms++;
                    if (rooms > max_rooms) break;
                }
            }
        bottom_room:
            vec2i bottom(x + w / 2, y);
            if (bottom.length() / max_y < 1)
            {
                int start = x - 1;
                int end = x - 1;
                for (int x0 = x + 1; x0 < x + w - 1; ++x0)
                {
                    if (start < x && isEmpty(map, vec2i(x0, y - 1)))
                    {
                        start = x0;
                    }
                    else if (start >= x && !isEmpty(map, vec2i(x0, y - 1)))
                    {
                        end = x0 - 1;
                        break;
                    }
                }
                if (start >= x)
                {
                    if (end < x) end = x + w - 1;
                    if (end - start < 3) goto left_room;
                    int door = rng.nextInt(start, end);
                    int nx = rng.nextInt(-8, -2);
                    int px = rng.nextInt(2, 8);
                    int ny = rng.nextInt(-8, -3);
                    int x0 = door;
                    for (; x0 >= door + nx; --x0)
                        if (!isEmpty(map, vec2i(x0, y - 1))) break;
                    int x1 = door;
                    for (; x1 <= door + px; ++x1)
                        if (!isEmpty(map, vec2i(x1, y - 1))) break;
                    if (x1 - x0 < 3) goto left_room;
                    int y0 = y - 1;
                    bool found = false;
                    for (; y0 > y + ny && !found; --y0)
                    {
                        for (int x2 = x0 + 1; x2 < x1; ++x2)
                        {
                            if (!isEmpty(map, vec2i(x2, y0)))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    if ((y - 1) - y0 < 3) goto left_room;
                    map.setTile(vec2i(door, y), Terrain::DirtFloor);
                    Room r(vec2i(x0, y0), vec2i(x1, y));
                    stack.push_front(r);
                    apply(map, r, 0);
                    rooms++;
                    if (rooms > max_rooms) break;
                }
            }
        left_room:
            vec2i left(x, y + h / 2);
            if (left.length() / max_x < 1)
            {
                int start = y - 1;
                int end = y - 1;
                for (int y0 = y + 1; y0 < y + h - 1; ++y0)
                {
                    if (start < y && isEmpty(map, vec2i(x - 1, y0)))
                    {
                        start = y0;
                    }
                    else if (start >= y && !isEmpty(map, vec2i(x - 1, y0)))
                    {
                        end = y0 - 1;
                        break;
                    }
                }
                if (start >= y)
                {
                    if (end < y) end = y + h - 1;
                    if (end - start < 3) goto right_room;
                    int door = rng.nextInt(start, end);
                    int nx = rng.nextInt(-8, -3);
                    int py = rng.nextInt(2, 8);
                    int ny = rng.nextInt(-8, -2);
                    int y0 = door;
                    for (; y0 >= door + ny; --y0)
                        if (!isEmpty(map, vec2i(x - 1, y0))) break;
                    int y1 = door;
                    for (; y1 <= door + py; ++y1)
                        if (!isEmpty(map, vec2i(x - 1, y1))) break;
                    if (y1 - y0 < 3) goto right_room;
                    int x0 = x - 1;
                    bool found = false;
                    for (; x0 > x + nx && !found; --x0)
                    {
                        for (int y2 = y0 + 1; y2 < y1; ++y2)
                        {
                            if (!isEmpty(map, vec2i(x0, y2)))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    if ((x - 1) - x0 < 3) goto right_room;
                    map.setTile(vec2i(x, door), Terrain::DirtFloor);
                    Room r(vec2i(x0, y0), vec2i(x, y1));
                    stack.push_front(r);
                    apply(map, r, 2);
                    rooms++;
                    if (rooms > max_rooms) break;
                }
            }
        right_room:
            vec2i right(x + w, y + h / 2);
            if (right.length() / max_x < 1)
            {
                int start = y - 1;
                int end = y - 1;
                for (int y0 = y + 1; y0 < y + h - 1; ++y0)
                {
                    if (start < y && isEmpty(map, vec2i(x + w, y0)))
                    {
                        start = y0;
                    }
                    else if (start >= y && !isEmpty(map, vec2i(x + w, y0)))
                    {
                        end = y0 - 1;
                        break;
                    }
                }
                if (start >= y)
                {
                    if (end < y) end = y + h - 1;
                    if (end - start < 3) continue;
                    int door = rng.nextInt(start, end);
                    int px = rng.nextInt(3, 8);
                    int py = rng.nextInt(2, 8);
                    int ny = rng.nextInt(-8, -2);
                    int y0 = door;
                    for (; y0 >= door + ny; --y0)
                        if (!isEmpty(map, vec2i(x + w, y0))) break;
                    int y1 = door;
                    for (; y1 <= door + py; ++y1)
                        if (!isEmpty(map, vec2i(x + w, y1))) break;
                    if (y1 - y0 < 3) continue;
                    int x0 = x + w;
                    bool found = false;
                    for (; x0 < x + w + px && !found; ++x0)
                    {
                        for (int y2 = y0 + 1; y2 < y1; ++y2)
                        {
                            if (!isEmpty(map, vec2i(x0, y2)))
                            {
                                found = true;
                                break;
                            }
                        }
                    }
                    if (x0 - (x + w) < 3) continue;
                    map.setTile(vec2i(x + w - 1, door), Terrain::DirtFloor);
                    Room r(vec2i(x + w - 1, y0), vec2i(x0, y1));
                    stack.push_front(r);
                    apply(map, r, 3);
                    rooms++;
                    if (rooms > max_rooms) break;
                }
            }
        }
    }
};

void generate(Map& map)
{
    if (map.name == "level_0")
    {
        DenseRoomGenerator gen;

        gen.generate(map);
    }
    else
    {
        debug_assert(false);
    }
}
