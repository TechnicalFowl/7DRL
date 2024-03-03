#include "universe.h"

#include <vector>

void UPlayer::update()
{

    pos += vel;
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

void Universe::update()
{
    std::vector<UActor*> moved;
    std::vector<vec2i> moved_last_pos;
    for (auto it : actors)
    {
        UActor* a = it.value;
        a->update();

        if (a->pos != it.key)
        {
            moved.push_back(a);
            moved_last_pos.push_back(it.key);
        }
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

}
