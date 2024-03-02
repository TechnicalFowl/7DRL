#pragma once

#include "util/direction.h"
#include "util/vector_math.h"

enum LayerPriority_
{
    LayerPriority_None = 0,
    LayerPriority_Background = 10,
    LayerPriority_Tiles = 20,
    LayerPriority_Objects = 30,
    LayerPriority_Actors = 50,
    LayerPriority_Particles = 70,
    LayerPriority_UI = 100,
    LayerPriority_Overlay = 200,
    LayerPriority_Debug = 1000,
};

enum SpecialChars : char
{
    Border_Vertical = 1,
    Border_Horizontal,
    Border_TopRight,
    Border_TopLeft,
    Border_BottomRight,
    Border_BottomLeft,
    Border_TeeLeft,
    Border_TeeUp,
    Border_TeeDown,
    Border_TeeRight,
    Border_Cross,
    Arrow_Up,
    Arrow_Down,
    Arrow_Left,
    Arrow_Right,
    Heart,
    LeftDiagTop,
    LeftDiagBottom,
    RightDiagTop,
    RightDiagBottom,
    LeftDiagTopInverse,
    LeftDiagBottomInverse,
    RightDiagTopInverse,
    RightDiagBottomInverse,
    FullChar,
    HalfTop,
    HalfBottom,

    MaxSpecialChars,
};

enum Tiles
{
    TileEmpty = 128,
    TileFull,
    TileCustom,
};

struct TextBuffer
{
    struct Char
    {
        int text[2]{ TileEmpty, TileEmpty };
        u32 color[2]{ 0, 0 };
        u32 bg = 0;
        int priority = -1;
        int bg_priority = -1;
        u32 overlay = 0;
        int overlay_priority = -1;
    };

    int w, h;
    Char* buffer = nullptr;

    TextBuffer(int w, int h);
    ~TextBuffer();

    void clear();
    void write(vec2i p, const char* text, u32 color, int priority = 0);
    void fillText(vec2i from, vec2i to, char c, u32 color, int prio = 0);
    void fill(vec2i from, vec2i to, int id, u32 color, int prio = 0);
    void fillBg(vec2i from, vec2i to, u32 color, int prio = 0);
    void setText(vec2i p, char c, u32 color, int priority = 0);
    void setTile(vec2i p, int id, u32 color, int priority = 0);
    void setBg(vec2i p, u32 color, int priority = 0);
    void setOverlay(vec2i p, u32 color, int priority = 0);
};
