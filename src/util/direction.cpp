#include "direction.h"

vec2i cardinals[4]
{
    vec2i(0, 1), // Up
    vec2i(1, 0), // Right
    vec2i(0, -1), // Down
    vec2i(-1, 0), // Left
};
Direction cardinal_dirs[4]
{
    Up,
    Right,
    Down,
    Left,
};

static vec2i directions[]
{
    vec2i(0, 0), // None
    vec2i(0, 1), // Up
    vec2i(1, 0), // Right
    vec2i(1, 1), // UpRight
    vec2i(0, -1), // Down
    vec2i(0, 0), // UpDown
    vec2i(1, -1), // DownRight
    vec2i(1, 0), // UpDownRight
    vec2i(-1, 0), // Left
    vec2i(-1, 1), // UpLeft
    vec2i(0, 0), // LeftRight
    vec2i(0, 1), // UpLeftRight
    vec2i(-1, -1), // DownLeft
    vec2i(-1, 0), // UpDownLeft
    vec2i(0, -1), // DownLeftRight
    vec2i(0, 0), // All
};
vec2i direction(Direction d) { return directions[d]; }

static Direction rotate_90_cw[]
{
    None, // None
    Right, // Up
    Down, // Right
    DownRight, // UpRight
    Left, // Down
    LeftRight, // UpDown
    DownLeft, // DownRight
    DownLeftRight, // UpDownRight
    Up, // Left
    UpRight, // UpLeft
    UpDown, // LeftRight
    UpDownRight, // UpLeftRight
    UpLeft, // DownLeft
    UpLeftRight, // UpDownLeft
    UpDownLeft, // DownLeftRight
    All, // All
};
Direction rotate90(Direction d) { return rotate_90_cw[d]; }

static Direction rotate_180[]
{
    None, // None
    Down, // Up
    Left, // Right
    DownLeft, // UpRight
    Up, // Down
    UpDown, // UpDown
    UpLeft, // DownRight
    UpDownLeft, // UpDownRight
    Right, // Left
    DownRight, // UpLeft
    LeftRight, // LeftRight
    DownLeftRight, // UpLeftRight
    UpRight, // DownLeft
    UpDownRight, // UpDownLeft
    UpLeftRight, // DownLeftRight
    All, // All
};
Direction rotate180(Direction d) { return rotate_180[d]; }

static Direction rotate_270_cw[]
{
    None, // None
    Left, // Up
    Up, // Right
    UpLeft, // UpRight
    Right, // Down
    LeftRight, // UpDown
    UpRight, // DownRight
    UpLeftRight, // UpDownRight
    Down, // Left
    DownLeft, // UpLeft
    UpDown, // LeftRight
    UpDownLeft, // UpLeftRight
    DownRight, // DownLeft
    DownLeftRight, // UpDownLeft
    UpDownRight, // DownLeftRight
    All, // All
};
Direction rotate270(Direction d) { return rotate_270_cw[d]; }

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

char getDirectionCharacter(Direction dir)
{
    switch (dir)
    {
        case Up: return '^';
        case Down: return 'v';
        case Left: return '<';
        case Right: return '>';
        default: return ' ';
    }
}

Direction getDirection(float rad)
{
    static Direction dirs[] = { Right, UpRight, Up, UpLeft, Left, DownLeft, Down, DownRight };
    return dirs[int((rad + scalar::PIf / 8) / (scalar::PIf / 4)) % 8];
}

Direction getDirection(vec2i dir)
{
    if (dir.x == 0)
    {
        if (dir.y == 0)     return None;
        else if (dir.y > 0) return Up;
        else                return Down;
    }
    else if (dir.y == 0)
    {
        if (dir.x > 0)      return Right;
        else                return Left;
    }
    else if (dir.x > 0)
    {
        if (dir.y > 0)      return UpRight;
        else                return DownRight;
    }
    else
    {
        if (dir.y > 0)      return UpLeft;
        else                return DownLeft;
    }
}

Direction getDirection(vec2i from, vec2i to)
{
    return getDirection(to - from);
}