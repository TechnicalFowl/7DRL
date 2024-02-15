#pragma once

#include "util/string.h"

#include "vterm.h"

enum class Action
{
    Wait,
    Move,
    Open,
    Attack,
    Zap,
    Pickup,
    Drop,
    Equip,
    Unequip,
    UseOn,
};

enum class Terrain
{
    Empty,
    StoneWall,
    DirtFloor,

    __COUNT,
    Invalid,
};

enum class ItemType
{
    Generic,
    Equipment,
    Weapon,

    __COUNT,
    Invalid,
};

enum class ActorType
{
    GroundItem,
    Player,
    Door,
    Goblin,

    __COUNT,
    Invalid,
};

struct TerrainInfo
{
    Terrain terrain = Terrain::Invalid;
    sstring name;

    int character = '?';
    u32 color = 0;
    u32 bg_color = 0;

    bool passable = true;

    TerrainInfo() {}
    TerrainInfo(Terrain terrain, const sstring& name, int character, u32 color, u32 bg_color, bool passable)
        : terrain(terrain), name(name), character(character), color(color), bg_color(bg_color), passable(passable) {}
};

struct ItemTypeInfo
{
    ItemType type = ItemType::Invalid;
    sstring name;

    int character = '?';
    u32 color = 0xFFFF00FF;

    ItemTypeInfo() {}
    ItemTypeInfo(ItemType t, const sstring& n, int ch, u32 col) : type(t), name(n), character(ch), color(col) {}
};

struct ActorInfo
{
    ActorType actor = ActorType::Invalid;
    sstring name;

    int character = '?';
    u32 color = 0xFFFF00FF;
    int priority = LayerPriority_Actors;

    bool is_ground = false;

    int max_health = 10;

    ActorInfo() {}
    ActorInfo(ActorType actor, const sstring& name, int character, u32 color, int priority, bool on_ground, int health)
        : actor(actor), name(name), character(character), color(color), priority(priority), is_ground(on_ground), max_health(health) {}
};

struct Registry
{
    TerrainInfo terrain_info[(int) Terrain::__COUNT];
    ActorInfo actor_info[(int) ActorType::__COUNT];
    ItemTypeInfo item_type_info[(int) ItemType::__COUNT];
};
