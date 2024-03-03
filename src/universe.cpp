#include "universe.h"

#include <vector>

void UPlayer::update()
{

    pos += vel;
}

void UPlayer::render(TextBuffer& buffer, vec2i origin)
{

    buffer.setTile(pos - origin, '@', 0xFFFFFFFF, LayerPriority_Actors);
}

vec2i getOffset(int& i, int& x, int& y)
{
    y++;
    if (y > i)
    {
        y = -i;
        x++;
    }
    while (true)
    {
        for (; x <= i; x++)
        {
            for (; y <= i; y++)
            {
                if (abs(x) == i || abs(y) == i)
                    return vec2i(x, y);
            }
            y = -i;
        }
        i++;
        x = -i;
        y = -i;
    }
}

void Universe::move(vec2i pos, UActor* a)
{
    vec2i p = pos;
    int i = 0;
    int x = 0;
    int y = 0;
    while (actors.find(p).found)
    {
        p = pos + getOffset(i, x, y);
    }
    actors.insert(a->pos, a);
}

void Universe::spawn(UActor* a)
{
    move(a->pos, a);
}

void Universe::update(vec2i origin)
{
    std::vector<vec2i> refresh_regions;
    for (auto it : regions_generated)
    {
        vec2i center(it.key.x << 5 + 16, it.key.y << 5 + 16);
        if ((center - origin).length() > 116.0f)
            refresh_regions.push_back(it.key);
    }
    for (vec2i r : refresh_regions) regions_generated.erase(r);

    for (int y = origin.y - 70; y < origin.y + 70; y += 32)
    {
        int ry = y >> 5;
        for (int x = origin.x - 70; x < origin.x + 70; x += 32)
        {
            int rx = x >> 5;

            if (!regions_generated.find(vec2i(rx, ry)).found)
            {
                // Populate region

                regions_generated.insert(vec2i(rx, ry), true);
            }
        }
    }

    std::vector<UActor*> moved;
    std::vector<UActor*> too_far;
    std::vector<vec2i> moved_last_pos;
    for (auto it : actors)
    {
        UActor* a = it.value;
        if ((a->pos - origin).length() > 100.0f)
        {
            too_far.push_back(a);
            continue;
        }
        a->update();

        if (a->pos != it.key)
        {
            moved.push_back(a);
            moved_last_pos.push_back(it.key);
        }
    }
    for (UActor* a : too_far)
    {
        delete a;
        actors.erase(a->pos);
    }
    int i = 0;
    for (UActor* a : moved)
    {
        actors.erase(moved_last_pos[i++]);
        move(a->pos, a);
    }
}

void Universe::render(TextBuffer& buffer, vec2i origin)
{
    vec2i bl = origin - vec2i(25, 22);
    for (auto it : actors)
    {
        it.value->render(buffer, bl);
    }
}
