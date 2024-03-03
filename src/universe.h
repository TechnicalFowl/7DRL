#pragma once

#include "util/linear_map.h"
#include "util/vector_math.h"

#include "vterm.h"

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
    virtual void render(TextBuffer& buffer, vec2i origin) = 0;
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

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct Universe
{
    linear_map<vec2i, UActor*> actors;
    linear_map<vec2i, bool> regions_generated;

    void move(vec2i p, UActor* a);
    void spawn(UActor* a);

    void update(vec2i origin);

    void render(TextBuffer& buffer, vec2i origin);
};
