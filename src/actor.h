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

    int count = 1;

    Item(int id, u32 color, ItemType type, const sstring& name) : character(id), color(color), type(type), name(name) {}
    Item(int id, u32 color, ItemType type, const sstring& name, int count) : character(id), color(color), type(type), name(name), count(count) {}

    sstring getName() const
    {
        sstring s = name;
        if (count > 1)
            s.appendf(" (x%d)", count);
        return s;
    }
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

struct ShipObject : Actor
{
    enum class Status
    {
        Active,
        Disabled,
        Damaged,
        Unpowered,
    };

    Status status = Status::Active;
    float power_required = 0.0f;

    ShipObject(vec2i p, ActorType t) : Actor(p, t) {}

};

extern const char* ShipObjectStatus[4];

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

struct PilotSeat : ShipObject
{
    PilotSeat(vec2i pos);
};

struct Scanner : ShipObject
{
    float range = 35.0f;

    Scanner(vec2i pos);
};

struct MainEngine : ShipObject
{
    MainEngine(vec2i pos);
};

struct Reactor : ShipObject
{
    float power = 0.0f;
    float capacity = 1000.0f;

    Reactor(vec2i pos);
};

struct TorpedoLauncher : ShipObject
{
    int torpedoes = 5;
    int charge_time = 0;

    int recharge_time = 5;
    int max_torpedoes = 5;

    TorpedoLauncher(vec2i pos);
};

struct PDC : ShipObject
{
    int rounds = 1000;
    float firing_variance = scalar::PIf / 2;

    int max_rounds = 1000;

    PDC(vec2i pos);
};

struct Railgun : ShipObject
{
    int rounds = 25;
    int charge_time = 0;

    int max_rounds = 25;
    int recharge_time = 4;
    float firing_variance = scalar::PIf / 10;

    Railgun(vec2i pos);
};
