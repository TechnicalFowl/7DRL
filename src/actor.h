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
extern const char* EquipmentSlotNames[EquipmentSlotCount];

enum class ModifierType
{
    Damage,
    Resistances,
    Accuracy,
    Dodge,
    Speed,
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

enum class WeaponType
{
    Melee,
    Ranged,

    __COUNT,
};
constexpr int WeaponTypeCount = int(WeaponType::__COUNT);
extern const char* WeaponTypeNames[WeaponTypeCount];

struct Weapon : Equipment
{
    WeaponType weapon_type;

    Weapon(int id, u32 color, ItemType type, const sstring& name, WeaponType wt)
        : Equipment(id, color, type, name, EquipmentSlot::MainHand), weapon_type(wt) {}
};

struct ActionData
{
    Action action;
    Actor* actor;
    float energy;
    union
    {
        vec2i move;
        Item* item;
    };

    ActionData(Action a, Actor* act, float e);
    ActionData(Action a, Actor* act, float e, vec2i p);
    ActionData(Action a, Actor* act, float e, Item* i);

    bool apply(Map& map, pcg32& rng);
};

struct Actor
{
    vec2i pos;
    ActorType type;

    float stored_energy = 0.0f;
    bool dead = false;

    Actor(vec2i pos, ActorType ty) : pos(pos), type(ty) {}

    virtual ActionData update(const Map& map, pcg32& rng, float dt) { stored_energy += dt; return ActionData(Action::Wait, this, dt); }

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

    std::vector<Item*> inventory;

    Player(vec2i pos);

    ActionData update(const Map& map, pcg32& rng, float dt) override { ActionData cpy = next_action; next_action = ActionData(Action::Wait, this, 0.0f); return cpy; }

    void tryMove(const Map& map, vec2i dir);
};

struct Monster : Living
{
    sstring name;

    Monster(vec2i pos, ActorType ty);

    ActionData update(const Map& map, pcg32& rng, float dt) override;
};

struct Door : Actor
{
    bool open = false;

    Door(vec2i pos);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};
