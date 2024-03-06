#include "procgen.h"

#include <deque>
#include <vector>

#include "stb_image.h"

#include "util/random.h"

#include "actor.h"
#include "game.h"
#include "map.h"
#include "ship.h"

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

void fillRoom(Map& map, vec2i min, vec2i max, Terrain t)
{
    for (int y = min.y; y <= max.y; ++y)
        for (int x = min.x; x <= max.x; ++x)
            map.setTile(vec2i(x, y), t);
}

void fillRoom(Map& map, vec2i min, vec2i max, Terrain t, Terrain w)
{
    for (int y = min.y; y <= max.y; ++y)
    {
        for (int x = min.x; x <= max.x; ++x)
        {
            if (x == min.x || x == max.x || y == min.y || y == max.y)
                map.setTile(vec2i(x, y), w);
            else
                map.setTile(vec2i(x, y), t);
        }
    }
}

void setDoor(Map& map, vec2i p)
{
    map.setTile(p, Terrain::ShipFloor);
    InteriorDoor* door = new InteriorDoor(p);
    map.spawn(door);
}

void setAirlock(Map& map, vec2i p, Direction d)
{
    map.setTile(p, Terrain::ShipFloor);
    Airlock* door = new Airlock(p, d);
    map.spawn(door);
}

void placeItem(Map& map, vec2i p, ItemType it)
{
    ItemTypeInfo& ii = g_game.reg.item_type_info[int(it)];
    Item* item = new Item(ii.character, ii.color, it, ii.name);
    GroundItem* ground_item = new GroundItem(p, item);
    map.spawn(ground_item);
}

void decorate(Map& map, vec2i p, int l, int r, u32 lc, u32 rc, u32 b)
{
    if (map.getTile(p) != Terrain::Empty)
        map.setTile(p, Terrain::ShipFloor);
    Decoration* dec = new Decoration(p, l, r, lc, rc, b);
    map.spawn(dec);
}

void fillDecorate(Map& map, vec2i min, vec2i max, int l, int r, u32 lc, u32 rc, u32 b)
{
    for (int y = min.y; y <= max.y; ++y)
        for (int x = min.x; x <= max.x; ++x)
            decorate(map, vec2i(x, y), l, r, lc, rc, b);
}

void placePilotSeat(Map& map, vec2i p)
{
    decorate(map, vec2i(p.x, p.y + 1), '=', '=', 0xFF00FF00, 0xFF00FF00, 0);

    PilotSeat* pilot_seat = new PilotSeat(p);
    map.spawn(pilot_seat);
}

void placeEngine(Map& map, vec2i p)
{
    decorate(map, vec2i(p.x - 1, p.y + 3), '/', Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 1, p.y + 3), Border_Horizontal, '\\', 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y + 2), Border_Vertical, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y + 2), Border_TopLeft, Border_TopRight, 0xFFFF5050, 0xFFFF4040, 0xFFFFFFFF);
    decorate(map, vec2i(p.x + 1, p.y + 2), 0, Border_Vertical, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y + 1), Border_Vertical, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y + 1), Border_TeeRight, Border_TeeLeft, 0xFFFF4040, 0xFFFF5050, 0xFFFFFFFF);
    decorate(map, vec2i(p.x + 1, p.y + 1), 0, Border_Vertical, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y), 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
    decorate(map, vec2i(p.x, p.y), Border_TeeRight, Border_TeeLeft, 0xFFFF5050, 0xFFFF4040, 0xFFFFFFFF);
    decorate(map, vec2i(p.x + 1, p.y), 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
    decorate(map, vec2i(p.x - 1, p.y - 1), '/', 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y - 1), Border_TeeRight, Border_TeeLeft, 0xFFFF5050, 0xFFFF4040, 0);
    decorate(map, vec2i(p.x + 1, p.y - 1), 0, '\\', 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 2, p.y - 2), 0, '/', 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 2, p.y - 2), '\\', 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);

    MainEngine* eng = new MainEngine(vec2i(p.x, p.y + 3));
    map.spawn(eng);
}

void placeSmallEngine(Map& map, vec2i p)
{
    decorate(map, vec2i(p.x - 1, p.y + 1), '/', Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 1, p.y + 1), Border_Horizontal, '\\', 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y), 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
    decorate(map, vec2i(p.x, p.y), Border_TopLeft, Border_TopRight, 0xFFFF5050, 0xFFFF4040, 0xFFFFFFFF);
    decorate(map, vec2i(p.x + 1, p.y), 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
    decorate(map, vec2i(p.x - 1, p.y - 1), Border_Vertical, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y - 1), Border_TeeRight, Border_TeeLeft, 0xFFFF5050, 0xFFFF4040, 0);
    decorate(map, vec2i(p.x + 1, p.y - 1), 0, Border_Vertical, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y - 2), '/', 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 1, p.y - 2), 0, '\\', 0xFFFFFFFF, 0xFFFFFFFF, 0);

    MainEngine* eng = new MainEngine(vec2i(p.x, p.y + 1));
    map.spawn(eng);
}

void placeReactor(Map& map, vec2i p)
{
    decorate(map, vec2i(p.x - 1, p.y + 2), LeftDiagTopInverse, Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 2, p.y + 2), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 1, p.y + 2), Border_Horizontal, RightDiagTopInverse, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 2, p.y + 2), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y + 1), Border_Vertical, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y + 1), Border_TopLeft, Border_TopRight, 0xFFFF5050, 0xFFFF4040, 0);
    decorate(map, vec2i(p.x + 1, p.y + 1), 0, Border_Vertical, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y), Border_Vertical, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y), Border_BottomLeft, Border_BottomRight, 0xFFFF4040, 0xFFFF5050, 0);
    decorate(map, vec2i(p.x + 1, p.y), 0, Border_Vertical, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 1, p.y - 1), LeftDiagBottomInverse, Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x - 2, p.y - 1), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x, p.y - 1), Border_Horizontal, Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 1, p.y - 1), Border_Horizontal, RightDiagBottomInverse, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    decorate(map, vec2i(p.x + 2, p.y - 1), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);

    Reactor* eng = new Reactor(vec2i(p.x, p.y + 2));
    map.spawn(eng);
}

void placeTorpedoLauncher(Map& map, vec2i p, Direction dir)
{
    if (dir == Up || dir == Down)
    {
        decorate(map, p + direction(dir), Border_Vertical, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        decorate(map, p + direction(dir) * 2, FullChar, FullChar, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        if (dir == Up)
        {
            decorate(map, vec2i(p.x - 1, p.y + 3), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x + 1, p.y + 3), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
        else
        {
            decorate(map, vec2i(p.x - 1, p.y - 3), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x + 1, p.y - 3), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
    }
    else
    {
        decorate(map, p + direction(dir), Border_Horizontal, Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        decorate(map, p + direction(dir) * 2, FullChar, FullChar, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        if (dir == Right)
        {
            decorate(map, vec2i(p.x + 3, p.y - 1), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x + 3, p.y + 1), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
        else
        {
            decorate(map, vec2i(p.x - 3, p.y - 1), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x - 3, p.y + 1), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
    }

    TorpedoLauncher* w = new TorpedoLauncher(p);
    map.spawn(w);
}

void placePDC(Map& map, vec2i p, Direction dir)
{
    switch (dir)
    {
    case Up:
    {
        decorate(map, vec2i(p.x - 1, p.y + 3), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        decorate(map, vec2i(p.x + 1, p.y + 3), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    } break;
    case Down:
    {
        decorate(map, vec2i(p.x - 1, p.y - 3), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        decorate(map, vec2i(p.x + 1, p.y - 3), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    } break;
    case Right:
    {
        decorate(map, vec2i(p.x + 3, p.y - 1), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        decorate(map, vec2i(p.x + 3, p.y + 1), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    } break;
    case Left:
    {
        decorate(map, vec2i(p.x - 3, p.y - 1), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        decorate(map, vec2i(p.x - 3, p.y + 1), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
    } break;
    }
    PDC* w = new PDC(p + direction(dir));
    map.spawn(w);
}

void placeRailgun(Map& map, vec2i p, Direction dir)
{
    if (dir == Up || dir == Down)
    {
        decorate(map, p + direction(dir), Border_Vertical, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        for (int i = 0; i < 6; ++i)
            decorate(map, p + direction(dir) * (i + 1), Border_Vertical, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        if (dir == Up)
        {
            decorate(map, vec2i(p.x - 1, p.y + 5), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x + 1, p.y + 5), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
        else
        {
            decorate(map, vec2i(p.x - 1, p.y - 5), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x + 1, p.y - 5), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
    }
    else
    {
        decorate(map, p + direction(dir), Border_Horizontal, Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        for (int i = 0; i < 6; ++i)
            decorate(map, p + direction(dir) * (i + 1), Border_Horizontal, Border_Horizontal, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        if (dir == Right)
        {
            decorate(map, vec2i(p.x + 5, p.y - 1), LeftDiagTop, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x + 5, p.y + 1), LeftDiagBottom, 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
        else
        {
            decorate(map, vec2i(p.x - 5, p.y - 1), 0, RightDiagTop, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            decorate(map, vec2i(p.x - 5, p.y + 1), 0, RightDiagBottom, 0xFFFFFFFF, 0xFFFFFFFF, 0);
        }
    }

    Railgun* w = new Railgun(p);
    map.spawn(w);
}

struct ShipParameters
{
    u32 primary_color = 0xFFFFFFFF;
    u32 secondary_color = 0xFF000000;

    int max_engines = 4;
    float engines = 0.5f;
    float engine_merge_chance = 0.7f;

    float extra_door_chance = 0.2f;

    int max_torpedos = 4;
    int max_pdcs = 4;
    int max_railguns = 2;
};

struct ShipGenerator
{
    int w, h;
    u32* outline;

    int size;

    struct Room
    {
        vec2i min, max;

        Room* adj[4]{ 0,0,0,0 };

        int placed_index = -1;

        Room(vec2i min, vec2i max)
            : min(min), max(max)
        {}

        bool used() { return placed_index != -1; }
    };

    struct PlacedRoom
    {
        vec2i min, max;
        RoomType type;
        Direction dir = Up;

        PlacedRoom(vec2i min, vec2i max, RoomType type)
            : min(min), max(max), type(type)
        {}
    };

    std::vector<Room> rooms;
    std::vector<Room> decorate_rooms;
    std::vector<PlacedRoom> placed_rooms;

    ShipGenerator(const char* shape_name, int size)
        : size(size)
    {
        int c;
        outline = (u32*)stbi_load(shape_name, &w, &h, &c, 4);
        for (int i = 0; i < w * h; i++)
        {
            if ((outline[i] & 0xFF000000) == 0)
            {
                outline[i] = 0;
            }
        }
    }
    ~ShipGenerator()
    {
        stbi_image_free(outline);
    }

    Room* getRoom(int x, int y)
    {
        for (Room& r : rooms)
        {
            if (x >= r.min.x && x <= r.max.x && y >= r.min.y && y <= r.max.y)
            {
                return &r;
            }
        }
        return nullptr;
    }

    Room* getRoom(Room* from, Direction dir)
    {
        vec2i d = direction(dir);
        vec2i next = from->min + d * (size + 1) + vec2i(size / 2, size / 2);
        return getRoom(next.x, next.y);
    }

    Room* getRoom(Room* from, vec2i d)
    {
        vec2i next = from->min + d * (size + 1) + vec2i(size / 2, size / 2);
        return getRoom(next.x, next.y);
    }

    Room* getRoom(Room* from, int dx, int dy)
    {
        return getRoom(from, vec2i(dx, dy));
    }

    bool check(float x, float y)
    {
        if (x < 0 || x >= w || y < 0 || y >= h) return false;
        int x0 = scalar::floori(x);
        int y0 = scalar::floori(y);
        if (x0 == x && y0 == y) return outline[y0 * w + x0] != 0;

        int x1 = x0 + 1;
        int y1 = y0 + 1;
        float dx = x - x0;
        float dy = y - y0;

        u32 c = outline[y0 * w + x0] != 0;
        c += outline[y0 * w + x1] != 0;
        c += outline[y1 * w + x0] != 0;
        c += outline[y1 * w + x1] != 0;

        return c >= 3;
    }

    void consume(Room* r, vec2i sz, int idx)
    {
        Room* cl = r;
        for (int x = 0; x < sz.x; ++x)
        {
            debug_assert(cl);
            cl->placed_index = idx;
            Room* cr = cl;
            for (int y = 1; y < sz.y; ++y)
            {
                cr = cr->adj[0];
                debug_assert(cr);
                cr->placed_index = idx;
            }
            cl = cl->adj[1];
        }
    }

    std::vector<Room*> find_candidates(vec2i sz)
    {
        std::vector<Room*> candidates;
        for (Room& r : rooms)
        {
            if (r.used()) continue;
            Room* cl = &r;
            bool valid = true;
            for (int x = 0; x < sz.x && valid; ++x)
            {
                if (!cl || cl->used())
                {
                    valid = false;
                    break;
                }
                Room* cr = cl;
                for (int y = 1; y < sz.y; ++y)
                {
                    cr = cr->adj[0];
                    if (!cr || cr->used())
                    {
                        valid = false;
                        break;
                    }
                }
                cl = cl->adj[1];
            }

            if (valid) candidates.push_back(&r);
        }
        return candidates;
    }

    Room* find_random(pcg32& rng, vec2i sz)
    {
        std::vector<Room*> candidates = find_candidates(sz);
        if (candidates.empty()) return nullptr;
        return candidates[(size_t) scalar::floori(candidates.size() * rng.nextFloat())];
    }

    bool generate(Ship* ship, Map& map, ShipParameters& params)
    {
        pcg32 rng;

        for (int y = 0; y + size + 1 < h; y += size + 1)
        {
            for (int x = 0; x + size + 1 < w; x += size + 1)
            {

                bool valid_room = true;
                bool needs_decorating = false;
                for (int y0 = y + 1; y0 <= y + size; ++y0)
                {
                    for (int x0 = x + 1; x0 <= x + size; ++x0)
                    {
                        if (outline[x0 + (h - y0 - 1) * w] == 0)
                        {
                            valid_room = false;
                        }
                        else
                        {
                            needs_decorating = true;
                        }
                    }
                }

                if (valid_room)
                {
                    rooms.emplace_back(vec2i(x, y), vec2i(x + size + 1, y + size + 1));
                }
                else if (needs_decorating)
                {
                    decorate_rooms.emplace_back(vec2i(x, y), vec2i(x + size + 1, y + size + 1));
                }
            }
        }

        std::deque<Room*> open;
        linear_map<vec2i, bool> seen;

        Room* center = getRoom(w / 2, h / 2);
        debug_assert(center);
        open.push_back(center);
        seen.insert(center->min, true);

        while (!open.empty())
        {
            Room* current = open.front();
            open.pop_front();
            for (int i = 0; i < 4; ++i)
            {
                vec2i d = cardinals[i];
                vec2i next = current->min + d * (size + 1) + vec2i(size/2,size/2);
                Room* nr = getRoom(next.x, next.y);
                if (!nr) continue;
                if (seen.find(nr->min).found) continue;
                open.push_back(nr);
                seen.insert(nr->min, true);
            }
        }

        for (auto it = rooms.begin(); it != rooms.end();)
        {
            Room& r = *it;
            if (seen.find(r.min).found)
            {
                //fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                ++it;
            }
            else
            {
                decorate_rooms.emplace_back(r);
                it = rooms.erase(it);
            }
        }

        // Rooms is frozen from ehre on!!!
        for (Room& r : rooms)
        {
            for (int i = 0; i < 4; ++i)
            {
                vec2i d = cardinals[i];
                vec2i next = r.min + d * (size + 1) + vec2i(size / 2, size / 2);
                Room* nr = getRoom(next.x, next.y);
                r.adj[i] = nr;
            }
        }

        // Engines
        std::vector<int> back_distance;
        for (int x = 0; x < w; ++x)
        {
            back_distance.push_back(h);
            for (int y = 0; y < h; ++y)
            {
                if (outline[x + (h - y - 1) * w])
                {
                    back_distance.back() = y;
                    break;
                }
            }
        }
        std::vector<Room*> possible_engines;
        for (int x = 2; x + size + 1 < w; x += size + 1)
        {
            for (int y = 2; y + size + 1 < h; y += size + 1)
            {
                Room* r = getRoom(x, y);
                if (r)
                {
                    if (!r->adj[0]) break;
                    bool valid = true;
                    for (int x0 = r->min.x + 1; x0 < r->max.x; ++x0)
                        if (back_distance[x0] < r->min.y - 2)
                            valid = false;
                    if (valid)
                        possible_engines.push_back(r);
                    break;
                }
            }
        }
        debug_assert(!possible_engines.empty());
        int engine_count = scalar::min(scalar::ceili(possible_engines.size() * params.engines), params.max_engines);
        while (engine_count > 0)
        {
            int next = scalar::floori(rng.nextFloat() * possible_engines.size());
            Room* r = possible_engines[next];
            debug_assert(r->adj[0]);
            if (engine_count > 1 && rng.nextFloat() < params.engine_merge_chance && r->adj[1] && std::find(possible_engines.begin(), possible_engines.end(), r->adj[1]) != possible_engines.end())
            {
                possible_engines.erase(std::find(possible_engines.begin(), possible_engines.end(), r));
                possible_engines.erase(std::find(possible_engines.begin(), possible_engines.end(), r->adj[1]));
                placed_rooms.emplace_back(r->min, r->adj[1]->adj[0]->max, RoomType::Engine);
                consume(r, vec2i(2, 2), (int) placed_rooms.size() - 1);
                engine_count -= 2;
            }
            else
            {
                possible_engines.erase(std::find(possible_engines.begin(), possible_engines.end(), r));
                placed_rooms.emplace_back(r->min, r->adj[0]->max, RoomType::Engine);
                consume(r, vec2i(1, 2), (int) placed_rooms.size() - 1);
                engine_count--;
            }
        }

        {
            Room* reactor = find_random(rng, vec2i(3, 3));
            debug_assert(reactor);
            placed_rooms.emplace_back(reactor->min, reactor->max + vec2i((size + 1) * 2, (size + 1) * 2), RoomType::Reactor);
            consume(reactor, vec2i(3, 3), (int) placed_rooms.size() - 1);
        }

        // Place 1 or 2 airlocks
        {
            std::vector<Room*> candidates = find_candidates(vec2i(1, 2));
            for (auto it = candidates.begin(); it != candidates.end();)
            {
                Room* r = *it;
                if (!r->adj[1] && !r->adj[0]->adj[1])
                {
                    ++it;
                }
                else if (!r->adj[3] && !r->adj[0]->adj[3])
                {
                    ++it;
                }
                else
                {
                    it = candidates.erase(it);
                }
            }
            if (candidates.empty())
            {
                // No candidates
                for (Room& r : rooms)
                {
                    if (r.used() || (r.adj[1] && r.adj[3])) continue;
                    placed_rooms.emplace_back(r.min, r.max, RoomType::Airlock);
                    consume(&r, vec2i(1, 1), (int) placed_rooms.size() - 1);
                }
            }
            else
            {
                int count = candidates.size() == 1 ? 1 : 1 + (rng.nextFloat() < 0.5f ? 1 : 0);
                for (int i = 0; i < count; ++i)
                {
                    auto it = candidates.begin() + (size_t) scalar::floori(rng.nextFloat() * candidates.size());
                    Room* r = *it;
                    if (r->used()) continue;
                    placed_rooms.emplace_back(r->min, r->adj[0]->max, RoomType::Airlock);
                    consume(r, vec2i(1, 2), (int)placed_rooms.size() - 1);
                    candidates.erase(it);
                }
            }
        }

        {
            Room* pilot_deck = find_random(rng, vec2i(2, 2));
            debug_assert(pilot_deck);
            placed_rooms.emplace_back(pilot_deck->min, pilot_deck->max + vec2i(size + 1, size + 1), RoomType::PilotsDeck);
            consume(pilot_deck, vec2i(2, 2), (int) placed_rooms.size() - 1);
        }

        {
            Room* operations_deck = find_random(rng, vec2i(2, 2));
            debug_assert(operations_deck);
            placed_rooms.emplace_back(operations_deck->min, operations_deck->max + vec2i(size + 1, size + 1), RoomType::OperationsDeck);
            consume(operations_deck, vec2i(2, 2), (int) placed_rooms.size() - 1);
        }

        int rem_torpedoes = params.max_torpedos > 1 ? rng.nextInt(1, params.max_torpedos) : params.max_torpedos;
        int rem_pdcs = params.max_pdcs > 1 ? rng.nextInt(1, params.max_pdcs) : params.max_pdcs;
        int rem_railguns = params.max_railguns > 1 ? rng.nextInt(1, params.max_railguns) : params.max_railguns;

        int total_weapons = rem_torpedoes + rem_pdcs + rem_railguns;

        std::vector<Room*> candidates;
        for (Room& r : rooms)
            if (!r.used())
                candidates.push_back(&r);

        while (total_weapons > 0 && !candidates.empty())
        {
            size_t ridx = (size_t) scalar::floori(rng.nextFloat() * candidates.size());
            Room& r = *candidates[ridx];
            for (int i = 0; i < 4; ++i)
            {
                if (!r.adj[i])
                {
                    Room* arc = getRoom(&r, cardinals[i] * 2);
                    if (arc) continue;
                    RoomType type;
                    if (r.adj[(i + 2) % 4] && !r.adj[(i + 2) % 4]->used())
                    {
                        float roll = rng.nextFloat();
                        if (roll < rem_torpedoes / float(total_weapons))
                        {
                            type = RoomType::TorpedoLauncher;
                            rem_torpedoes--;
                            total_weapons--;
                        }
                        else if (roll < (rem_torpedoes + rem_pdcs) / float(total_weapons))
                        {
                            type = RoomType::PDC;
                            rem_pdcs--;
                            total_weapons--;
                        }
                        else
                        {
                            type = RoomType::Railgun;
                            rem_railguns--;
                            total_weapons--;
                        }
                    }
                    else
                    {
                        if (rng.nextFloat() < rem_torpedoes / float(rem_torpedoes + rem_pdcs))
                        {
                            type = RoomType::TorpedoLauncher;
                            rem_torpedoes--;
                            total_weapons--;
                        }
                        else
                        {
                            type = RoomType::PDC;
                            rem_pdcs--;
                            total_weapons--;
                        }
                    }
                    if (type == RoomType::Railgun)
                    {
                        debug_assert(r.adj[(i + 2) % 4]);
                        Room& o = *r.adj[(i + 2) % 4];
                        placed_rooms.emplace_back(min(r.min, o.min), max(r.max, o.max), type);
                        r.placed_index = (int)placed_rooms.size() - 1;
                        o.placed_index = (int)placed_rooms.size() - 1;
                        candidates.erase(candidates.begin() + ridx);
                        candidates.erase(std::find(candidates.begin(), candidates.end(), &o));
                    }
                    else
                    {
                        placed_rooms.emplace_back(r.min, r.max, type);
                        r.placed_index = (int) placed_rooms.size() - 1;
                        candidates.erase(candidates.begin() + ridx);
                    }
                    placed_rooms.back().dir = cardinal_dirs[i];
                    break;
                }
            }
        }

        vec2i size_progression[]{vec2i(3,3), vec2i(3,2), vec2i(2,3), vec2i(2,2)};
        for (vec2i sz : size_progression)
        {
            Room* n = find_random(rng, sz);
            while (n)
            {
                placed_rooms.emplace_back(n->min, n->max + vec2i((size + 1) * (sz.x - 1), (size+1) * (sz.y - 1)), RoomType::StorageRoom);
                consume(n, sz, (int) placed_rooms.size() - 1);
                n = find_random(rng, sz);
            }
        }

        for (Room& r : rooms)
        {
            if (r.used()) continue;

            int cw = 1;
            Room* h = &r;
            while (h->adj[1])
            {
                h = h->adj[1];
                if (h->used()) break;
                ++cw;
            }
            int ch = 1;
            Room* v = &r;
            while (v->adj[0])
            {
                v = v->adj[0];
                if (v->used()) break;
                ++ch;
            }
            if (cw > 1 && cw > ch)
            {
                placed_rooms.emplace_back(r.min, r.max + vec2i((size + 1) * (cw - 1), 0), RoomType::Coridor);
                consume(&r, vec2i(cw, 1), (int) placed_rooms.size() - 1);
            }
            else if(ch > 1)
            {
                placed_rooms.emplace_back(r.min, r.max + vec2i(0, (size + 1) * (ch - 1)), RoomType::Coridor);
                consume(&r, vec2i(1, ch), (int) placed_rooms.size() - 1);
            }
        }

        for (Room& r : rooms)
        {
            if (r.used()) continue;
            placed_rooms.emplace_back(r.min, r.max, RoomType::StorageRoom);
            r.placed_index = (int) placed_rooms.size() - 1;
        }

        for (PlacedRoom& r : placed_rooms)
        {
            switch (r.type)
            {
            case RoomType::Engine:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                if (r.max.x - r.min.x > size * 2)
                    placeEngine(map, vec2i((r.min.x + r.max.x) / 2, r.min.y));
                else
                    placeSmallEngine(map, vec2i((r.min.x + r.max.x) / 2, r.min.y));
            } break;
            case RoomType::Reactor:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                placeReactor(map, r.min + vec2i((size + 1) * 3 / 2, (size + 1) * 3 / 2));
            } break;
            case RoomType::PilotsDeck:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                placePilotSeat(map, r.min + vec2i(size + 1, size));
            } break;
            case RoomType::OperationsDeck:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                decorate(map, r.min + vec2i(2, 2), 'O', 0, 0xFFFFFFFF, 0xFFFFFFFF, 0);
            } break;
            case RoomType::Airlock:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                Room* l = getRoom(r.min.x + size + 2, r.min.y +1);
                if (!l)
                    setAirlock(map, vec2i(r.min.x + size + 1, r.min.y + size / 2 + 1), Left);
                l = getRoom(r.min.x -1, r.min.y +1);
                if (!l)
                    setAirlock(map, vec2i(r.min.x, r.min.y + size / 2 + 1), Right);
            } break;
            case RoomType::StorageRoom:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                vec2i sz = (r.max - r.min) / (size + 1);
                if (sz.x == 1 && sz.y == 1)
                {
                    static ItemType storage_items[]
                    {
                        ItemType::RepairParts,
                        ItemType::Torpedoes,
                        ItemType::RailgunRounds,
                        ItemType::PDCRounds,
                    };
                    ItemType item = storage_items[rng.nextInt(0, 3)];
                    
                    if (rng.nextFloat() < 0.8f) placeItem(map, r.min + vec2i(1, 1), item);
                    if (rng.nextFloat() < 0.8f) placeItem(map, r.min + vec2i(size, 1), item);
                    if (rng.nextFloat() < 0.8f) placeItem(map, r.min + vec2i(1, size), item);
                    if (rng.nextFloat() < 0.8f) placeItem(map, r.min + vec2i(size, size), item);
                }
            } break;
            case RoomType::TorpedoLauncher:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                placeTorpedoLauncher(map, (r.min + r.max) / 2, r.dir);
            } break;
            case RoomType::PDC:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                placePDC(map, (r.min + r.max) / 2, r.dir);
            } break;
            case RoomType::Railgun:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
                placeRailgun(map, (r.min + r.max) / 2, r.dir);
            } break;
            default:
            {
                fillRoom(map, r.min, r.max, Terrain::ShipFloor, Terrain::ShipWall);
            } break;
            }
        }

        for (PlacedRoom& r : placed_rooms)
        {
            ShipRoom& sr = ship->rooms.emplace_back(r.min, r.max, r.type);

            linear_map<vec2i, bool> connected;
            for (int x = r.min.x + 1 + size / 2; x < r.max.x; x += size + 1)
            {
                Room* other_room = getRoom(x, r.min.y - 1);
                if (other_room)
                {
                    PlacedRoom& other = placed_rooms[other_room->placed_index];
                    if (r.type == RoomType::Coridor && other.type == RoomType::Coridor)
                    {
                        for (int x0 = x - size / 2; x0 <= x + size / 2; ++x0)
                        {
                            map.setTile(vec2i(x0, r.min.y), Terrain::ShipFloor);
                        }
                    }
                    else if (!connected.find(other.min).found || rng.nextFloat() < params.extra_door_chance)
                    {
                        connected.insert(other.min, true);
                        if (r.type == RoomType::Airlock)
                        {
                            setAirlock(map, vec2i(x, r.min.y), Up);
                        }
                        else if (other.type == RoomType::Airlock)
                        {
                            setAirlock(map, vec2i(x, r.min.y), Down);
                        }
                        else
                        {
                            setDoor(map, vec2i(x, r.min.y));
                        }
                    }
                }
            }
            for (int y = r.min.y + 1 + size / 2; y < r.max.y; y += size + 1)
            {
                Room* other_room = getRoom(r.min.x - 1, y);
                if (other_room)
                {
                    PlacedRoom& other = placed_rooms[other_room->placed_index];
                    if (r.type == RoomType::Coridor && other.type == RoomType::Coridor)
                    {
                        for (int y0 = y - size / 2; y0 <= y + size / 2; ++y0)
                        {
                            map.setTile(vec2i(r.min.x, y0), Terrain::ShipFloor);
                        }
                    }
                    else if (!connected.find(other.min).found || rng.nextFloat() < params.extra_door_chance)
                    {
                        connected.insert(other.min, true);
                        if (r.type == RoomType::Airlock)
                        {
                            setAirlock(map, vec2i(r.min.x, y), Up);
                        }
                        else if (other.type == RoomType::Airlock)
                        {
                            setAirlock(map, vec2i(r.min.x, y), Down);
                        }
                        else
                        {
                            setDoor(map, vec2i(r.min.x, y));
                        }
                    }
                }
            }
        }

        for (Room& r : decorate_rooms)
        {
            for (int x = r.min.x; x <= r.max.x; ++x)
            {
                for (int y = r.min.y; y <= r.max.y; ++y)
                {
                    if (outline[x + (h - y - 1) * w] && map.getTile(vec2i(x, y)) == Terrain::Empty)
                    {
                        map.setTile(vec2i(x, y), Terrain::ShipFloor);
                    }
                }
            }
        }
        return true;
    }
};

Ship* generate(const sstring& name, const char* type)
{
    pcg32 rng;
    static const char* ship_shapes[]
    {
        "ship_0.png",
        "ship_1.png",
        "ship_2.png",
        "ship_3.png",
        "ship_4.png",
        "ship_5.png",
        "ship_6.png",
    };

    if (strings::equals(type, "player_ship"))
    {
        Map* map = new Map(name);
        Ship* ship = new Ship(map);

        ShipGenerator shape(ship_shapes[rng.nextInt(0, 7)], 3);
        ShipParameters params;
        params.primary_color = 0xFF14CCFF;
        params.secondary_color = 0xFFC0C0C0;

        bool success = false;
        for (int a = 0; a < 10; ++a)
        {
            if (shape.generate(ship, *map, params))
            {
                success = true;
                break;
            }
        }
        debug_assertf(success, "Failed to generate ship layout within 10 attempts");

        ShipRoom* pilot = ship->getRoom(RoomType::PilotsDeck);

        Player* player = new Player(pilot->min + vec2i(2, 2));
        map->player = player;
        map->spawn(player);
        return ship;
    }
    else if (strings::equals(type, "cargo_ship"))
    {
        Map* map = new Map(name);
        Ship* ship = new Ship(map);

        ShipGenerator shape(ship_shapes[rng.nextInt(0, 7)], 3);
        ShipParameters params;
        params.primary_color = 0xFF14CCFF;
        params.secondary_color = 0xFFC0C0C0;
        params.max_pdcs = 6;
        params.max_railguns = 0;
        params.max_torpedos = 0;

        bool success = false;
        for (int a = 0; a < 10; ++a)
        {
            if (shape.generate(ship, *map, params))
            {
                success = true;
                break;
            }
        }
        debug_assertf(success, "Failed to generate ship layout within 10 attempts");
        return ship;
    }
    else if (strings::equals(type, "pirate_ship"))
    {
        Map* map = new Map(name);
        Ship* ship = new Ship(map);

        ShipGenerator shape(ship_shapes[rng.nextInt(0, 7)], 3);
        ShipParameters params;
        params.primary_color = 0xFFFFCC14;
        params.secondary_color = 0xFFC0C0C0;
        params.max_pdcs = 2;
        params.max_railguns = 1;
        params.max_torpedos = 4;

        bool success = false;
        for (int a = 0; a < 10; ++a)
        {
            if (shape.generate(ship, *map, params))
            {
                success = true;
                break;
            }
        }
        debug_assertf(success, "Failed to generate ship layout within 10 attempts");
        return ship;
    }
    debug_assertf(false, "Unknown ship layout type");
    return nullptr;
}
