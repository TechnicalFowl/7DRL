#include "actor.h"

#include "util/random.h"

#include "game.h"
#include "map.h"

ActionData::ActionData(Action a, Actor* act)
    : action(a), actor(act)
{
    move = vec2i(0, 0);
}

ActionData::ActionData(Action a, Actor* act, vec2i p)
    : action(a), actor(act)
{
    move = p;
}


bool ActionData::apply(Map& map)
{
    static vec2i dirs[] = { vec2i(0, 1), vec2i(0, -1), vec2i(1, 0), vec2i(-1, 0) };
    switch (action)
    {
    case Action::Move:
    {
        vec2i target = actor->pos + move;
        if (!map.move(actor, actor->pos + move))
        {
            if (actor == map.player) g_game.log.log("That way is blocked.");
            return false;
        }
        return true;
    } break;
    case Action::Open:
    {
        if (move.zero())
        {
            for (int i = 0; i < 4; ++i)
            {
                auto it = map.tiles.find(actor->pos + dirs[i]);
                if (!it.found) continue;
                if (!it.value.actor) continue;
                if (it.value.actor->type == ActorType::Door)
                {
                    Door* door = (Door*)it.value.actor;
                    door->open = !door->open;
                    if (actor == map.player) g_game.log.logf("You %s the door.", door->open ? "open" : "close");
                    return true;
                }
            }
            if (actor == map.player) g_game.log.log("There is nothing to open.");
            return false;
        }
        else
        {
            auto it = map.tiles.find(actor->pos + move);
            if (it.found)
            {
                if (it.value.actor)
                {
                    if (it.value.actor->type == ActorType::Door)
                    {
                        Door* door = (Door*)it.value.actor;
                        door->open = !door->open;
                        if (actor == map.player) g_game.log.logf("You %s the door.", door->open ? "open" : "close");
                        return true;
                    }
                }
            }
            if (actor == map.player) g_game.log.log("There is nothing to open?");
            return false;
        }
        vec2i target = actor->pos + move;
        return map.move(actor, actor->pos + move);
    } break;
    case Action::Attack:
    {
        auto it = map.tiles.find(actor->pos + move);
        if (it.found)
        {
            if (it.value.actor)
            {
                Actor* target = it.value.actor;
                ActorInfo& ai = g_game.reg.actor_info[int(target->type)];
                if (actor == map.player) g_game.log.logf("You attack the %s.", ai.name);
                return true;
            }
        }
        if (actor == map.player) g_game.log.log("There is nothing to attack?");
        return false;
    } break;
    default:
    {
        g_game.log.logf(0xFFFF0000, "Err: Invalid action %d", (int)action);
    } break;
    }
    return false;
}

void Actor::render(TextBuffer& buffer, vec2i origin, bool dim)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    u32 col = dim ? scalar::convertToGrayscale(ai.color, 0.5f) : ai.color;
    buffer.setTile(pos - origin, ai.character, col, ai.priority);
}

GroundItem::GroundItem(vec2i pos, const Item& item)
    : Actor(pos, ActorType::GroundItem)
    , item(item)
{

}

void GroundItem::render(TextBuffer& buffer, vec2i origin, bool dim)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    ItemTypeInfo& iti = g_game.reg.item_type_info[int(item.type)];
    u32 col = item.color ? item.color : iti.color;
        col = dim ? scalar::convertToGrayscale(col, 0.5f) : col;
    buffer.setTile(pos - origin, item.character ? item.character : iti.character, col, ai.priority);
}

Player::Player(vec2i pos)
    : Actor(pos, ActorType::Player)
    , next_action(Action::Wait, this)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    health = ai.max_health;
}

void Player::tryMove(const Map& map, vec2i dir)
{
    debug_assert(dir.length() == 1);
    auto it = map.tiles.find(pos + dir);
    if (!it.found)
    {
        next_action = ActionData(Action::Move, this, dir);
        return;
    }
    else
    {
        TerrainInfo& ti = g_game.reg.terrain_info[int(it.value.terrain)];
        if (!ti.passable)
        {
            next_action = ActionData(Action::Wait, this);
            return;
        }
        if (it.value.actor)
        {
            if (it.value.actor->type == ActorType::Door)
            {
                next_action = ActionData(Action::Open, this, dir);
                return;
            }
            else
            {
                next_action = ActionData(Action::Attack, this, dir);
                return;
            }
        }
    }
    next_action = ActionData(Action::Move, this, dir);
}

Monster::Monster(vec2i pos, ActorType ty)
    : Actor(pos, ty)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    health = ai.max_health;
}

Door::Door(vec2i pos)
    : Actor(pos, ActorType::Door)
{

}

void Door::render(TextBuffer& buffer, vec2i origin, bool dim)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    u32 col = dim ? scalar::convertToGrayscale(ai.color, 0.5f) : ai.color;
    buffer.setTile(pos - origin, open ? '.' : '#', col, ai.priority);
}

ActionData Monster::update(const Map& map, pcg32& rng)
{
    int dir = rng.nextInt(0, 8);
    if (dir == 0 && map.isPassable(pos + vec2i(1, 0)))
        return ActionData(Action::Move, this, vec2i(1, 0));
    else if (dir == 1 && map.isPassable(pos + vec2i(-1, 0)))
        return ActionData(Action::Move, this, vec2i(-1, 0));
    else if (dir == 2 && map.isPassable(pos + vec2i(0, 1)))
        return ActionData(Action::Move, this, vec2i(0, 1));
    else if (dir == 3 && map.isPassable(pos + vec2i(0, -1)))
        return ActionData(Action::Move, this, vec2i(0, -1));

    return ActionData(Action::Wait, this);
}
