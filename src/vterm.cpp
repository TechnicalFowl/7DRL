#include "vterm.h"

TextBuffer::TextBuffer(int w, int h)
    : w(w), h(h)
{
    buffer = new Char[w * h];
}

TextBuffer::~TextBuffer()
{
    delete[] buffer;
}

void TextBuffer::clear()
{
    for (int i = 0; i < w * h; ++i)
        buffer[i] = Char();
}

void TextBuffer::write(vec2i p, const char* text, u32 color, int priority)
{
    char n;
    while (n = *text++)
    {
        setText(p, n, color, priority);
        p.x++;
    }
}

void TextBuffer::fillText(vec2i from, vec2i to, char c, u32 color, int prio)
{
    for (int y = from.y; y <= to.y; ++y)
    {
        for (int x = from.x; x <= to.x; ++x)
        {
            setText(vec2i(x, y), c, color, prio);
        }
    }
}

void TextBuffer::fill(vec2i from, vec2i to, int id, u32 color, int prio)
{
    for (int y = from.y; y <= to.y; ++y)
    {
        for (int x = from.x; x <= to.x; ++x)
        {
            setTile(vec2i(x, y), id, color, prio);
        }
    }
}

void TextBuffer::fillBg(vec2i from, vec2i to, u32 color, int prio)
{
    for (int y = from.y; y <= to.y; ++y)
    {
        for (int x = from.x; x <= to.x; ++x)
        {
            setBg(vec2i(x, y), color, prio);
        }
    }
}

void TextBuffer::setText(vec2i p, char c, u32 color, int priority)
{
    if (p.x < 0 || p.x >= w * 2 || p.y < 0 || p.y >= h) return;
    int sub_x = p.x % 2;
    Char& ch = buffer[p.x / 2 + p.y * w];
    if (ch.priority <= priority)
    {
        ch.text[sub_x] = c;
        if (sub_x == 0 && ch.text[1] == 0xFFFF) ch.text[1] = 0;
        ch.color = color;
        ch.priority = priority;
    }
}

void TextBuffer::setTile(vec2i p, int id, u32 color, int priority)
{
    if (p.x < 0 || p.x >= w || p.y < 0 || p.y >= h) return;
    Char& ch = buffer[p.x + p.y * w];
    if (ch.priority < priority)
    {
        ch.text[0] = id; ch.text[1] = 0xFFFF;
        ch.color = color;
        ch.priority = priority;
    }
}

void TextBuffer::setBg(vec2i p, u32 color, int priority)
{
    if (p.x < 0 || p.x >= w || p.y < 0 || p.y >= h) return;
    Char& ch = buffer[p.x + p.y * w];
    if (ch.bg_priority < priority)
    {
        ch.bg = color;
        ch.bg_priority = priority;
    }
    if (ch.priority < priority)
    {
        ch.text[0] = TileEmpty; ch.text[1] = TileEmpty;
        ch.priority = priority;
    }
}
