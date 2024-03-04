#include "universe.h"

#include <vector>

#include "game.h"

void UCargoShip::update(pcg32& rng)
{
    if (vel.length() < 2)
    {
        vel += vec2i(rng.nextInt(-1, 2), rng.nextInt(-1, 2));
    }
    float speed = vel.length();
    if (speed > 0)
    {
        for (int i = 1; i <= 3; ++i)
        {
            if (g_game.universe->hasActor(pos + vel * i))
            {
                vel = vec2i((int)round(vel.x / speed), (int)round(vel.y / speed));
                break;
            }
        }
    }
}

void UCargoShip::render(TextBuffer& buffer, vec2i origin)
{
    buffer.setTile(pos - origin, 'C', 0xFFFFFFFF, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x800000FF, LayerPriority_Overlay);
    }
}

void UPlayer::update(pcg32& rng)
{
}

void UPlayer::render(TextBuffer& buffer, vec2i origin)
{
    buffer.setTile(pos - origin, '@', 0xFFFFFFFF, LayerPriority_Actors);
    if (!vel.zero())
    {
        buffer.setOverlay((pos + vel) - origin, 0x8000FF00, LayerPriority_Overlay);
    }
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

void Universe::move(UActor* a, vec2i d)
{
    debug_assert(a->type == UActorType::Player || a->type == UActorType::CargoShip);
    bool target_occupied = actors.find(a->pos + d).found;

    float x0 = a->pos.x + 0.5f;
    float y0 = a->pos.y + 0.5f;
    float x1 = a->pos.x + d.x + 0.5f;
    float y1 = a->pos.y + d.y + 0.5f;

    float dx = abs(x1 - x0);
    float dy = abs(y1 - y0);

    int x = a->pos.x;
    int y = a->pos.y;
    int lx = x;
    int ly = y;

    int n = scalar::floori(dx + dy);
    int x_inc = (x1 > x0) ? 1 : -1;
    int y_inc = (y1 > y0) ? 1 : -1;

    float error = dx - dy;
    dx *= 2;
    dy *= 2;
    bool warned_this_step = false;
    for (; n > 0; --n)
    {
        if (error > 0)
        {
            x += x_inc;
            error -= dy;
        }
        else
        {
            y += y_inc;
            error += dx;
        }

        auto it = actors.find(vec2i(x, y));
        if (it.found)
        {
            UShip* pl = (UShip*)a;
            if (pl->vel.length() > 6)
            {
                if (a == g_game.uplayer)
                {
                    g_game.log.log("You crash your ship at such a speed that there is only dust left from the impact.");
                    g_game.log.log("");
                    g_game.log.log("Game over.");
                    g_game.state = GameState::GameOver;
                }
                else
                    a->dead = true;
                return;
            }
            else if (it.value->type == UActorType::Asteroid)
            {
                if (pl->vel.length() > 2)
                {
                    if (a == g_game.uplayer) g_game.log.log("Impact alert!");
                    // TODO: damage ship from asteroid hit
                }
                else if (!warned_this_step)
                {
                    if (a == g_game.uplayer) g_game.log.log("Collision avoidance activated!");
                    warned_this_step = true;
                }
                pl->vel = vec2i(0, 0);
                break;
            }
            else if (!target_occupied)
            {
                if (!warned_this_step)
                {
                    if (a == g_game.uplayer) g_game.log.log("Collision avoidance activated!");
                    warned_this_step = true;
                }
            }
            else
            {
                if (!warned_this_step)
                {
                    if (a == g_game.uplayer) g_game.log.log("Collision avoidance activated!");
                    warned_this_step = true;
                }
                pl->vel = vec2i(0, 0);
                break;
                }
        }
        lx = x;
        ly = y;
    }
    if (a->pos != vec2i(lx, ly))
    {
        actors.erase(a->pos);
        a->pos = vec2i(lx, ly);
        actors.insert(a->pos, a);
    }
}

void Universe::spawn(UActor* a)
{
    vec2i p = a->pos;
    int i = 0;
    int x = 0;
    int y = 0;
    while (actors.find(p).found)
    {
        p = a->pos + getOffset(i, x, y);
    }
    actors.insert(p, a);
    a->pos = p;

    if (a->type == UActorType::Asteroid)
    {
        UAsteroid* ast = (UAsteroid*)a;
        float step = scalar::PIf / (4 * ast->radius);
        for (float a = 0; a + step / 2 < scalar::PIf * 2; a += step)
        {
            float r = (ast->radius + cos(a * 1.2 + ast->sfreq) * ast->radius);
            vec2i p = ast->pos + vec2i((int)round(cos(a) * r), (int)round(sin(a) * r));
            for (int r0 = 0; r0 < r; ++r0)
            {
                vec2i p0 = ast->pos + vec2i((int)round(cos(a) * r0), (int)round(sin(a) * r0));
                if (actors.find(p0).found) continue;
                actors.insert(p0, (UActor*) ast);
            }
            if (!actors.find(p).found)
                actors.insert(p, (UActor*)ast);
        }
    }
}

void Universe::update(vec2i origin)
{
    universe_ticks++;
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
                        if (rng.nextFloat() < 0.05f)
                        {
                            UAsteroid* a = new UAsteroid(vec2i((rx << 5) + (x0 << 3), (ry << 5) + (y0 << 3)));
                            a->sfreq = rng.nextFloat() * 10;
                            a->radius = 1.0f + rng.nextFloat() * 2.0f + rng.nextFloat() * rng.nextFloat() * 8.0f;
                            spawn(a);
                        }
                        else
                        {
                            if (rng.nextFloat() < 0.01f)
                            {
                                UCargoShip* s = new UCargoShip(vec2i((rx << 5) + (x0 << 4) + 2, (ry << 5) + (y0 << 3)));
                                spawn(s);
                            }
                        }
                    }
                }

                regions_generated.insert(vec2i(rx, ry), true);
            }
        }
    }

    std::vector<UShip*> moved;
    std::vector<UActor*> to_remove;
    for (auto it : actors)
    {
        UActor* a = it.value;
        if (a->pos != it.key) continue; // Proxy actor
        if ((a->pos - origin).length() > 160.0f)
        {
            to_remove.push_back(a);
            continue;
        }
        a->update(rng);

        if (a->dead)
        {
            to_remove.push_back(a);
        }
        else if (a->type == UActorType::Player || a->type == UActorType::CargoShip)
        {
            moved.push_back((UShip*) a);
        }
    }
    for (UShip* a : moved)
    {
        if (!a->vel.zero())
        {
            move(a, a->vel);
            if (a->dead)
            {
                to_remove.push_back(a);
            }
        }
    }
    for (UActor* a : to_remove)
    {
        actors.erase(a->pos);
        if (a->type == UActorType::Asteroid)
        {
            // Remove proxies
            UAsteroid* ast = (UAsteroid*)a;
            float r2 = ast->radius;
            for (int y = -r2; y <= r2; ++y)
            {
                for (int x = -r2; x <= r2; ++x)
                {
                    vec2i p = a->pos + vec2i(x, y);
                    auto it = actors.find(p);
                    if (it.found && it.value == a)
                        actors.erase(p);
                }
            }
        }
        delete a;
    }
}

void Universe::render(TextBuffer& buffer, vec2i origin)
{
    vec2i bl = origin - vec2i(25, 22);
    for (auto it : actors)
    {
        if (it.value->pos != it.key)
            buffer.setBg(it.key - bl, 0xFF303030, LayerPriority_Background);
        else
            it.value->render(buffer, bl);
    }
}
