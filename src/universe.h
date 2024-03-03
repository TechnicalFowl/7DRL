#pragma once

#include "util/linear_map.h"
#include "util/vector_math.h"

#include "vterm.h"

constexpr int UniverseRegionSize = 32;
struct Ship;

enum class UActorType
{
    Player,
};

struct UActor
{
    UActorType type;
    vec2i pos;

    UActor(UActorType type, vec2i p) : type(type), pos(p) {}

    virtual void update() {};
};

struct UShip : UActor
{
    vec2i vel;
    Ship* ship;

    UShip(UActorType t, vec2i p) : UActor(t, p) {}
};

struct UPlayer : UShip
{

    UPlayer(vec2i p) : UShip(UActorType::Player, p) {}

    void update() override;
};

struct Universe
{
    linear_map<vec2i, UActor*> actors;

    void move(vec2i p, UActor* a);
    void spawn(UActor* a);

    void update();

    void render(TextBuffer& buffer, vec2i origin);
};
