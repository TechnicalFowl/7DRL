#pragma once

#include <vector>

#include "vector_math.h"

enum Direction : u8
{
    None = 0b0000,
    Up = 0b0001,
    Right = 0b0010,
    UpRight = 0b0011,
    Down = 0b0100,
    UpDown = 0b0101,
    DownRight = 0b0110,
    UpDownRight = 0b0111,
    Left = 0b1000,
    UpLeft = 0b1001,
    LeftRight = 0b1010,
    UpLeftRight = 0b1011,
    DownLeft = 0b1100,
    UpDownLeft = 0b1101,
    DownLeftRight = 0b1110,
    All = 0b1111,
};

extern vec2i cardinals[4];
extern Direction cardinal_dirs[4];

vec2i direction(Direction d);
Direction rotate90(Direction d);
Direction rotate180(Direction d);
Direction rotate270(Direction d);
vec2i rotateCW(vec2i p, Direction dir);
vec2i rotateCCW(vec2i p, Direction dir);

char getDirectionCharacter(Direction dir);
Direction getDirection(float rad);
Direction getDirection(vec2i dir);
Direction getDirection(vec2i from, vec2i to);

std::vector<vec2i> findRay(vec2i from, vec2i to);
