#include "ship.h"

ShipRoom* Ship::getRoom(vec2i p)
{
    for (auto& r : rooms)
    {
        if (p.x >= r.min.x && p.x <= r.max.x && p.y >= r.min.y && p.y <= r.max.y)
            return &r;
    }
    return nullptr;
}
