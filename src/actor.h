#pragma once

#include <vector>

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

enum class DamageType
{
    Blunt,
    Piercing,
    Slashing,
    Fire,
    Cold,
    Acid,
    Poison,

    __COUNT,
};
constexpr int DamageTypeCount = int(DamageType::__COUNT);

enum class EquipmentSlot
{
    MainHand,
    OffHand,

    Head,
    Body,
    Legs,
    Feet,

    __COUNT,
};
constexpr int EquipmentSlotCount = int(EquipmentSlot::__COUNT);

enum class ModifierType
{
    Damage,
    Resistances,
    Accuracy,
    Dodge,
};

struct EquipmentModifier
{
    ModifierType type;
    DamageType dmg;
    float flat;
    float percent;

    EquipmentModifier(ModifierType type, DamageType dmg, float flat, float percent)
        : type(type), dmg(dmg), flat(flat), percent(percent) {}
    EquipmentModifier(ModifierType type, float flat, float percent)
        : type(type), dmg(DamageType::Blunt), flat(flat), percent(percent) {}
};

struct Equipment : Item
{
    EquipmentSlot slot;
    std::vector<EquipmentModifier> modifiers;

    Equipment(int id, u32 color, ItemType type, const sstring& name, EquipmentSlot slot)
        : Item(id, color, type, name), slot(slot) {}
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

    bool apply(Map& map, pcg32& rng);
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
    Item* item;

    GroundItem(vec2i pos, Item* item);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};

struct Living : Actor
{
    int health;
    int max_health;
    std::vector<EquipmentModifier> inate_modifiers;
    Equipment* equipment[EquipmentSlotCount]{ nullptr };

    Living(vec2i pos, ActorType type);
};

struct Player : Living
{
    ActionData next_action;

    Player(vec2i pos);

    ActionData update(const Map& map, pcg32& rng) override { return next_action; }

    void tryMove(const Map& map, vec2i dir);
};

struct Monster : Living
{
    sstring name;

    Monster(vec2i pos, ActorType ty);

    ActionData update(const Map& map, pcg32& rng) override;
};

struct Door : Actor
{
    bool open = false;

    Door(vec2i pos);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};
