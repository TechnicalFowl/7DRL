#include "universe.h"

#include <vector>

#include "game.h"

void UPlayer::update()
{

    pos += vel;
}

void UPlayer::render(TextBuffer& buffer, vec2i origin)
{

    buffer.setTile(pos - origin, '@', 0xFFFFFFFF, LayerPriority_Actors);
}

void UAsteroid::render(TextBuffer& buffer, vec2i origin)
{
    float step = scalar::PIf / (4 * radius);
    for (float a = 0; a + step/2 < scalar::PIf * 2; a += step)
    {
        float r = (radius + cos(a * 1.2 + sfreq) * radius);
        vec2i p = pos + vec2i((int) round(cos(a) * r), (int) round(sin(a) * r));
        for (int r0 = 0; r0 < r; ++r0)
            buffer.setTile(pos + vec2i((int)round(cos(a) * r0), (int)round(sin(a) * r0)) - origin, '0', 0xFF505050, LayerPriority_Background-1);
        buffer.setTile(p - origin, getProjectileCharacter(rotate90(getDirection(a))), 0xFFFFFFFF, LayerPriority_Background);
    }
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
        vec2i center((it.key.x << 5) + 16, (it.key.y << 5) + 16);
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
                for (int y0 = 0; y0 < 4; ++y0)
                {
                    for (int x0 = 0; x0 < 4; ++x0)
                    {
                        if (rng.nextFloat() < 0.1f)
                        {
                            UAsteroid* a = new UAsteroid(vec2i((rx << 5) + (x0 << 3), (ry << 5) + (y0 << 3)));
                            a->sfreq = rng.nextFloat() * 10;
                            a->radius = 1.0f + rng.nextFloat() * 2.0f + rng.nextFloat() * rng.nextFloat() * 8.0f;
                            spawn(a);
                        }
                    }
                }

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
        if ((a->pos - origin).length() > 160.0f)
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
        actors.erase(a->pos);
        delete a;
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
