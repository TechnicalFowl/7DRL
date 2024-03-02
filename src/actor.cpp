#include "actor.h"

#include <deque>

#include "util/random.h"

#include "game.h"
#include "map.h"

#if 0
const char* EquipmentSlotNames[EquipmentSlotCount]
{
    "Main",
    "Off",

    "Head",
    "Body",
    "Legs",
    "Feet",
};

const char* WeaponTypeNames[WeaponTypeCount]
{
    "Melee",
    "Ranged",
};

struct OffenseStats
{
    float speed = 1.0f;
    float accuracy = 0.0f;
    int damage[DamageTypeCount]{ 0, 0, 0, 0, 0, 0, 0 };
};

OffenseStats getOffense(Living* attacker)
{
    OffenseStats stats;
    float flat_accuracy = 0.0f;
    float mult_accuracy = 1.0f;
    float flat_dmg[DamageTypeCount]{ 0, 0, 0, 0, 0, 0, 0 };
    float mult_dmg[DamageTypeCount]{ 1, 1, 1, 1, 1, 1, 1 };

    for (auto& mod : attacker->inate_modifiers)
    {
        switch (mod.type)
        {
        case ModifierType::Accuracy:
        {
            flat_accuracy += mod.flat;
            mult_accuracy *= mod.percent;
        } break;
        case ModifierType::Speed:
        {
            stats.speed *= mod.percent;
        } break;
        case ModifierType::Damage:
        {
            flat_dmg[int(mod.dmg)] += mod.flat;
            mult_dmg[int(mod.dmg)] *= mod.percent;
        } break;
        default:
            break;
        }
    }

    for (int i = 0; i < EquipmentSlotCount; ++i)
    {
        Equipment* e = attacker->equipment[i];
        if (!e) continue;

        for (auto& mod : e->modifiers)
        {
            switch (mod.type)
            {
            case ModifierType::Accuracy:
            {
                flat_accuracy += mod.flat;
                mult_accuracy *= mod.percent;
            } break;
            case ModifierType::Speed:
            {
                stats.speed *= mod.percent;
            } break;
            case ModifierType::Damage:
            {
                flat_dmg[int(mod.dmg)] += mod.flat;
                mult_dmg[int(mod.dmg)] *= mod.percent;
            } break;
            default:
                break;
            }
        }
    }

    stats.accuracy = scalar::max(flat_accuracy * mult_accuracy, 0);
    for (int i = 0; i < DamageTypeCount; ++i)
    {
        stats.damage[i] = scalar::max(scalar::floori(flat_dmg[i] * mult_dmg[i]), 0);
    }
    return stats;
}

struct DefenceStats
{
    float dodge = 0.0f;
    float resistances[DamageTypeCount]{ 1, 1, 1, 1, 1, 1, 1 };
    float armor[DamageTypeCount]{ 0, 0, 0, 0, 0, 0, 0 };
};

DefenceStats getDefence(Living* target)
{
    DefenceStats stats;
    float flat_dodge = 0.0f;
    float mult_dodge = 1.0f;

    for (auto& mod : target->inate_modifiers)
    {
        switch (mod.type)
        {
        case ModifierType::Dodge:
        {
            flat_dodge += mod.flat;
            mult_dodge *= mod.percent;
        } break;
        case ModifierType::Damage:
        {
            stats.armor[int(mod.dmg)] += mod.flat;
            stats.resistances[int(mod.dmg)] *= mod.percent;
        } break;
        default:
            break;
        }
    }

    for (int i = 0; i < EquipmentSlotCount; ++i)
    {
        Equipment* e = target->equipment[i];
        if (!e) continue;

        for (auto& mod : e->modifiers)
        {
            switch (mod.type)
            {
            case ModifierType::Dodge:
            {
                flat_dodge += mod.flat;
                mult_dodge *= mod.percent;
            } break;
            case ModifierType::Damage:
            {
                stats.armor[int(mod.dmg)] += mod.flat;
                stats.resistances[int(mod.dmg)] *= mod.percent;
            } break;
            default:
                break;
            }
        }
    }

    stats.dodge = scalar::max(flat_dodge * mult_dodge, 0);
    return stats;
}

int calculateDamage(Living* attacker, Living* target, pcg32& rng)
{
    OffenseStats off = getOffense(attacker);
    DefenceStats def = getDefence(target);

    float hit_chance = off.accuracy / (off.accuracy + def.dodge);
    if (hit_chance < rng.nextFloat()) return 0;

    int damage = 0;
    for (int i = 0; i < DamageTypeCount; ++i)
    {
        if (off.damage[i] == 0) continue;
        int dmg = scalar::floori((off.damage[i] - def.armor[i]) * def.resistances[i]);
        damage += scalar::max(dmg, 0);
    }
    return damage;
}
#endif

ActionData::ActionData(Action a, Actor* act, float e)
    : action(a), actor(act), energy(e)
{
    item = nullptr;
}

ActionData::ActionData(Action a, Actor* act, float e, vec2i p)
    : action(a), actor(act), energy(e)
{
    move = p;
}

ActionData::ActionData(Action a, Actor* act, float e, Item* i)
    : action(a), actor(act), energy(e)
{
    item = i;
}

bool ActionData::apply(Map& map, pcg32& rng)
{
    actor->stored_energy -= energy;
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
    case Action::Pickup:
    {
        debug_assert(actor->type == ActorType::Player);
        Player* pl = (Player*)actor;
        auto it = map.tiles.find(actor->pos);
        if (it.found)
        {
            if (it.value.ground && it.value.ground->type == ActorType::GroundItem)
            {
                GroundItem* item = (GroundItem*)it.value.ground;
                if (pl->holding)
                {
                    std::swap(item->item, pl->holding);
                    g_game.log.logf("You drop the %s and pickup the %s.", item->item->name.c_str(), pl->holding->name.c_str());
                }
                else
                {
                    pl->holding = item->item;
                    item->dead = true;
                    g_game.log.logf("You pickup the %s.", pl->holding->name.c_str());
                }
            }
            else if (!it.value.ground && pl->holding)
            {
                GroundItem* item = new GroundItem(pl->pos, pl->holding);
                map.spawn(item);
                pl->holding = nullptr;
                g_game.log.logf("You drop the %s.", item->item->name.c_str());
            }
        }
    } break;
    case Action::Open:
    {
        if (move.zero())
        {
            for (int i = 0; i < 4; ++i)
            {
                auto it = map.tiles.find(actor->pos + dirs[i]);
                if (!it.found) continue;
                if (it.value.ground)
                {
                    if (it.value.ground->type == ActorType::InteriorDoor || it.value.ground->type == ActorType::Airlock)
                    {
                        move = dirs[i];
                        break;
                    }
                }
            }
            if (move.zero())
            {
                if (actor == map.player) g_game.log.log("There is nothing to interact with.");
                return false;
            }
        }
        auto it = map.tiles.find(actor->pos + move);
        if (it.found)
        {
            if (it.value.ground)
            {
                if (it.value.ground->type == ActorType::InteriorDoor)
                {
                    InteriorDoor* door = (InteriorDoor*)it.value.ground;
                    if (door->welded)
                    {
                        if (actor == map.player) g_game.log.log("The door is welded shut.");
                        return false;
                    }
                    door->open = !door->open;
                    if (actor == map.player) g_game.log.logf("You %s the door.", door->open ? "open" : "close");
                    return true;
                }
                else if (it.value.ground->type == ActorType::Airlock)
                {
                    Airlock* door = (Airlock*)it.value.ground;
                    if (door->welded)
                    {
                        if (actor == map.player) g_game.log.log("The airlock is welded shut.");
                        return false;
                    }
                    door->open = !door->open;
                    if (door->open)
                    {
                        Airlock* opp = door->findOpposite(map);
                        if (opp)
                        {
                            if (actor == map.player) g_game.log.logf("The airlock cycles.");
                            opp->open = false;
                        }
                    }
                    if (actor == map.player) g_game.log.logf("You %s the airlock.", door->open ? "open" : "close");
                    return true;
                }
            }
        }
        if (actor == map.player) g_game.log.log("There is nothing to interact with");
        return false;
    } break;
#if 0
    case Action::Zap:
    {
        auto path = map.findRay(actor->pos, move);
        g_game.animations.push_back(new ProjectileAnimation(actor->pos, move, 0xFFFFFFFF));
        for (vec2i p : path)
        {
            if (actor->pos == p) continue;
            auto it = map.tiles.find(p);
            if (it.found)
            {
                if (it.value.actor)
                {
                    Actor* target = it.value.actor;
                    ActorInfo& ti = g_game.reg.actor_info[int(target->type)];
                    ActorInfo& ai = g_game.reg.actor_info[int(actor->type)];

                    if (target->dead) break;

                    switch (target->type)
                    {
                    case ActorType::Player:
                    case ActorType::Goblin:
                    {
                        Living* l_actor = (Living*)actor;
                        Living* l_target = (Living*)target;

                        int damage = calculateDamage(l_actor, l_target, rng);
                        if (damage > 0)
                        {
                            l_target->health -= damage;
                            if (l_target->health <= 0)
                            {
                                if (actor == map.player)
                                {
                                    g_game.log.logf("You kill the %s.", ti.name);
                                    target->dead = true;
                                }
                                if (target == map.player)
                                {
                                    g_game.log.logf("The %s kills you.", ai.name);
                                    g_game.state = GameState::GameOver;
                                }
                            }
                            else
                            {
                                if (actor == map.player) g_game.log.logf("You attack the %s dealing %d damage.", ti.name, damage);
                                if (target == map.player) g_game.log.logf("The %s attacks you dealing %d damage.", ai.name, damage);
                            }
                        }
                        else
                        {
                            if (actor == map.player) g_game.log.logf("You attack the %s but miss.", ti.name);
                            if (target == map.player) g_game.log.logf("The %s attacks you but misses.", ai.name);
                        }
                        return true;
                    }
                    default: break;
                    }
                }
            }
        }
        if (actor == map.player) g_game.log.log("Your arrow doesn't hit anything.");
    } break;
#endif
    case Action::UseOn:
    {
        debug_assert(actor->type == ActorType::Player);
        Player* pl = (Player*)actor;
        if (!pl->holding)
        {
            g_game.log.log("You aren't holding anything.");
            return false;
        }
        ItemType item = pl->holding->type;
        if (move.zero())
        {
            for (int i = 0; i < 4 && move.zero(); ++i)
            {
                auto it = map.tiles.find(actor->pos + dirs[i]);
                if (!it.found) continue;
                switch (item)
                {
                case ItemType::WeldingTorch:
                {
                    if (it.value.ground)
                    {
                        if (it.value.ground->type == ActorType::InteriorDoor || it.value.ground->type == ActorType::Airlock)
                        {
                            move = dirs[i];
                        }
                    }
                } break;
                default: break;
                }
            }
            if (move.zero())
            {
                if (actor == map.player) g_game.log.log("There is nothing to use that with.");
                return false;
            }
        }
        auto it = map.tiles.find(actor->pos + move);
        if (it.found)
        {
            if (it.value.ground)
            {
                if (it.value.ground->type == ActorType::InteriorDoor)
                {
                    InteriorDoor* door = (InteriorDoor*)it.value.ground;
                    switch (item)
                    {
                    case ItemType::WeldingTorch:
                    {
                        bool was_open = door->open;
                        door->open = false;
                        door->welded = !door->welded;
                        if (actor == map.player) g_game.log.logf("You %s the door.", was_open ? "close and weld" : (door->welded ? "weld shut" : "unweld"));
                        return true;
                    } break;
                    default: break;
                    }
                }
                else if (it.value.ground->type == ActorType::Airlock)
                {
                    Airlock* door = (Airlock*)it.value.ground;
                    switch (item)
                    {
                    case ItemType::WeldingTorch:
                    {
                        bool was_open = door->open;
                        door->open = false;
                        door->welded = !door->welded;
                        if (actor == map.player) g_game.log.logf("You %s the airlock.", was_open ? "close and weld" : (door->welded ? "weld shut" : "unweld"));
                        return true;
                    } break;
                    default: break;
                    }
                    return true;
                }
            }
        }
        debug_assert(false); // The surroundings check earlier should have caught this
        g_game.log.log("That doesn't seem to do anything.");
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

GroundItem::GroundItem(vec2i pos, Item* item)
    : Actor(pos, ActorType::GroundItem)
    , item(item)
{

}

void GroundItem::render(TextBuffer& buffer, vec2i origin, bool dim)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    ItemTypeInfo& iti = g_game.reg.item_type_info[int(item->type)];
    u32 col = item->color ? item->color : iti.color;
        col = dim ? scalar::convertToGrayscale(col, 0.5f) : col;
    buffer.setTile(pos - origin, item->character ? item->character : iti.character, col, ai.priority);
}

Living::Living(vec2i p, ActorType ty)
    : Actor(p, ty)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    health = ai.max_health;
    max_health = ai.max_health;
}

Player::Player(vec2i pos)
    : Living(pos, ActorType::Player)
    , next_action(Action::Wait, this, 0.0f)
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
        next_action = ActionData(Action::Move, this, 1.0f, dir);
        return;
    }
    else
    {
        TerrainInfo& ti = g_game.reg.terrain_info[int(it.value.terrain)];
        if (!ti.passable)
        {
            next_action = ActionData(Action::Wait, this, 0.0f);
            return;
        }
        if (it.value.ground)
        {
            if (it.value.ground->type == ActorType::InteriorDoor)
            {
                InteriorDoor* door = (InteriorDoor*)it.value.ground;
                if (!door->open)
                {
                    next_action = ActionData(Action::Open, this, 1.0f, dir);
                    return;
                }
            }
            else if (it.value.ground->type == ActorType::Airlock)
            {
                Airlock* door = (Airlock*) it.value.ground;
                if (!door->open)
                {
                    next_action = ActionData(Action::Open, this, 1.0f, dir);
                    return;
                }
            }
        }
        if (it.value.actor)
        {
            next_action = ActionData(Action::Open, this, 1.0f, dir);
            return;
        }
    }
    next_action = ActionData(Action::Move, this, 1.0f, dir);
}

InteriorDoor::InteriorDoor(vec2i pos)
    : Actor(pos, ActorType::InteriorDoor)
{

}

void InteriorDoor::render(TextBuffer& buffer, vec2i origin, bool dim)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    u32 col = dim ? scalar::convertToGrayscale(ai.color, 0.5f) : ai.color;
    buffer.setTile(pos - origin, open ? '.' : '#', col, ai.priority);
}

Airlock::Airlock(vec2i pos, Direction i)
    : Actor(pos, ActorType::Airlock)
    , interior(i)
{

}

void Airlock::render(TextBuffer& buffer, vec2i origin, bool dim)
{
    ActorInfo& ai = g_game.reg.actor_info[int(type)];
    u32 col = dim ? scalar::convertToGrayscale(ai.color, 0.5f) : ai.color;
    buffer.setTile(pos - origin, open ? '.' : '#', col, ai.priority);
}

Airlock* Airlock::findOpposite(Map& map)
{
    std::deque<vec2i> open;
    linear_map<vec2i, bool> seen;

    open.push_back(pos + direction(interior));

    while (!open.empty())
    {
        vec2i current = open.front();
        open.pop_front();
        for (vec2i d : cardinals)
        {
            vec2i next = current + d;
            if (seen.find(next).found) continue;
            auto it = map.tiles.find(next);
            if (!it.found) continue;
            if (!g_game.reg.terrain_info[(int)it.value.terrain].passable) continue;
            if (it.value.ground && it.value.ground->type == ActorType::Airlock)
            {
                if (it.value.ground == this) continue;
                return (Airlock*)it.value.ground;
            }
            if (!map.isPassable(next)) continue;

            seen.insert(next, true);
            open.push_back(next);
        }
    }
    return nullptr;
}

#if 0

Monster::Monster(vec2i pos, ActorType ty)
    : Living(pos, ty)
{
}

ActionData Monster::update(const Map& map, pcg32& rng, float dt)
{
    stored_energy += dt;
    OffenseStats stats = getOffense(this);
    bool lack_energy = false;
    if (stored_energy >= 1.0f / stats.speed)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto it = map.tiles.find(pos + direction(Direction(i)));
            if (it.found && it.value.actor && it.value.actor->type == ActorType::Player)
                return ActionData(Action::Attack, this, 1.0f / stats.speed, direction(Direction(i)));
        }
    }
    else
        lack_energy = true;

    int dir = rng.nextInt(0, 8);
    if (stored_energy >= 1.0f)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (dir == i && map.isPassable(pos + direction(Direction(i))))
                return ActionData(Action::Move, this, 1.0f, direction(Direction(i)));
        }
    }
    else
        lack_energy = true;

    return ActionData(Action::Wait, this, lack_energy ? 0.0f : dt);
}
#endif