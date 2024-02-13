#pragma once

#include "util/string.h"

#include "types.h"

struct Actor;
struct Map;
struct pcg32;

struct Item
{
    int character;
    u32 color;
    ItemType type;
    sstring name;

    Item(int id, u32 color, ItemType type, const sstring& name) : character(id), color(color), type(type), name(name) {}
};

struct ActionData
{
    Action action;
    Actor* actor;
    union
    {
        vec2i move;
    };

    ActionData(Action a, Actor* act);
    ActionData(Action a, Actor* act, vec2i p);

    bool apply(Map& map);
};

struct Actor
{
    vec2i pos;
    ActorType type;

    Actor(vec2i pos, ActorType ty) : pos(pos), type(ty) {}

    virtual ActionData update(const Map& map, pcg32& rng) { return ActionData(Action::Wait, this); }

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim=false);
};

struct GroundItem : Actor
{
    Item item;

    GroundItem(vec2i pos, const Item& item);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};

struct Player : Actor
{
    ActionData next_action;

    int health = 10;

    Player(vec2i pos);

    ActionData update(const Map& map, pcg32& rng) override { return next_action; }

    void tryMove(const Map& map, vec2i dir);
};

struct Monster : Actor
{
    sstring name;
    int health = 10;

    Monster(vec2i pos, ActorType ty);

    ActionData update(const Map& map, pcg32& rng) override;
};

struct Door : Actor
{
    bool open = false;

    Door(vec2i pos);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};
