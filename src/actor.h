#pragma once

#include <vector>

#include "util/string.h"

#include "types.h"

struct Actor;
struct Map;
struct Ship;
struct pcg32;

struct Item
{
    int character;
    u32 color;
    ItemType type;
    sstring name;

    Item(int id, u32 color, ItemType type, const sstring& name) : character(id), color(color), type(type), name(name) {}
};

#if 0
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
#endif

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

    bool apply(Ship* ship, pcg32& rng);
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
#if 0
    std::vector<EquipmentModifier> inate_modifiers;
    Equipment* equipment[EquipmentSlotCount]{ nullptr };
#endif

    Living(vec2i pos, ActorType type);
};

struct Player : Living
{
    ActionData next_action;
    bool is_aiming = false;

    Item* holding = nullptr;

    Player(vec2i pos);

    ActionData update(const Map& map, pcg32& rng, float dt) override { ActionData cpy = next_action; next_action = ActionData(Action::Wait, this, 0.0f); return cpy; }

    void tryMove(const Map& map, vec2i dir);
};

#if 0
struct Monster : Living
{
    sstring name;

    Monster(vec2i pos, ActorType ty);

    ActionData update(const Map& map, pcg32& rng, float dt) override;
};
#endif

struct Decoration : Actor
{
    int left, right;
    u32 leftcolor, rightcolor;
    u32 bg;

    Decoration(vec2i pos, int l, int r, u32 lc, u32 rc, u32 b);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};

struct InteriorDoor : Actor
{
    bool open = false;
    bool welded = false;

    InteriorDoor(vec2i pos);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};

struct Airlock : Actor
{
    Direction interior;
    bool open = false;
    bool welded = false;

    Airlock(vec2i pos, Direction i);

    virtual void render(TextBuffer& buffer, vec2i origin, bool dim = false) override;
};

struct PilotSeat : Actor
{
    bool active = false;

    PilotSeat(vec2i pos);
};

struct MainEngine : Actor
{
    float thrust = 0.0f;

    MainEngine(vec2i pos);
};

struct Reactor : Actor
{
    float power = 0.0f;

    Reactor(vec2i pos);
};

struct TorpedoLauncher : Actor
{
    bool open = false;
    int torpedoes = 0;

    TorpedoLauncher(vec2i pos);
};

struct PDC : Actor
{
    bool active = false;
    int rounds = 0;

    PDC(vec2i pos);
};

struct Railgun : Actor
{
    bool open = false;
    int rounds = 0;

    Railgun(vec2i pos);
};
