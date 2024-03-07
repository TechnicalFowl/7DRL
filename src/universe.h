#pragma once

#include "util/linear_map.h"
#include "util/random.h"
#include "util/vector_math.h"

#include "vterm.h"

struct Ship;

enum class UActorType
{
    Player,
    Asteroid,
    CargoShip,
    Torpedo,
    PirateShip,
    Station,
};

struct UActor
{
    u32 id = 0;
    UActorType type;
    vec2i pos;

    bool dead = false;

    UActor(UActorType type, vec2i p) : type(type), pos(p) {}
    virtual ~UActor() {}

    virtual void update(pcg32& rng) {}
    virtual void render(TextBuffer& buffer, vec2i origin) = 0;
};

struct UAsteroid : UActor
{
    float sfreq = 0.5f;
    float radius = 2.0f;

    UAsteroid(vec2i p) : UActor(UActorType::Asteroid, p) {}

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UShip : UActor
{
    vec2i vel;
    Ship* ship = nullptr;

    UShip(UActorType t, vec2i p) : UActor(t, p) {}
    ~UShip();

    virtual void update(pcg32& rng) override;

    bool fireTorpedo(vec2i target);
    bool fireRailgun(vec2i target);
};

struct UCargoShip : UShip
{
    UCargoShip(vec2i p);

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UPirateShip : UShip
{
    u32 target = 0;
    vec2i target_last_pos;
    int check_for_target = 10;

    int torp_reload_cooldown = 10;
    int torp_max_reloads = 5;

    int railgun_reload_cooldown = 10;
    int railgun_max_reloads = 4;

    UPirateShip(vec2i p);

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UPlayer : UShip
{
    bool is_aiming = false;
    bool is_aiming_railgun = false;

    UPlayer(vec2i p) : UShip(UActorType::Player, p) {}

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UTorpedo : UShip
{
    vec2i target_pos;
    u32 target = 0;
    u32 source = 0;

    UTorpedo(vec2i p) : UShip(UActorType::Torpedo, p) {}

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UStation : UActor
{
    u32 upgrades = 0;

    int repair_cost = 3;
    int scrap_price = 3;

    UStation(vec2i p);

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;

};

struct UShipWreck : UActor
{
    int scrap;

    UShipWreck(vec2i p);

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct ULostTrack
{
    vec2i pos;
    vec2i vel;
    u32 color;
    u32 id;

    ULostTrack(vec2i p, vec2i v, u32 c, u32 i) : pos(p), vel(v), color(c), id(i) {}
};

struct Universe
{
    linear_map<u32, UActor*> actor_ids;
    linear_map<vec2i, UActor*> actors;
    linear_map<vec2i, bool> regions_generated;
    linear_map<u32, ULostTrack> lost_tracks;

    pcg32 rng;
    int universe_ticks = 0;
    u32 next_actor = 1;

    bool hasActor(vec2i p) { return actors.find(p).found; }

    bool isVisible(vec2i from, vec2i to);
    bool checkArea(vec2i pos, int radius);

    void move(UActor* a, vec2i d);
    void spawn(UActor* a);

    void update(vec2i origin);

    void render(TextBuffer& buffer, vec2i origin);
};
