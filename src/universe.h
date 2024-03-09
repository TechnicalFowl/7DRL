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
    ShipWreck,
    MilitaryStation,

    __COUNT,
};
constexpr int UActorTypeCount = int(UActorType::__COUNT);
extern const char* UActorTypeNames[UActorTypeCount];

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

    u32 color;
    u32 inner_color;

    UAsteroid(vec2i p, u32 c, u32 ic) : UActor(UActorType::Asteroid, p), color(c), inner_color(ic) {}

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UShip : UActor
{
    vec2i vel;
    Ship* ship = nullptr;

    bool animating = false;

    UShip(UActorType t, vec2i p) : UActor(t, p) {}
    ~UShip();

    virtual void update(pcg32& rng) override;

    bool fireTorpedo(vec2i target, int power);
    bool fireRailgun(vec2i target, int power);
};

struct UCargoShip : UShip
{
    UCargoShip(vec2i p);

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UPirateShip : UShip
{
    int character;
    u32 color;

    u32 target = 0;
    vec2i target_last_pos;
    int check_for_target = 10;

    int torp_reload_cooldown = 10;
    int torp_max_reloads = 5;

    int railgun_reload_cooldown = 10;
    int railgun_max_reloads = 4;

    bool has_alerted = false;
    bool has_warned = false;

    int railgun_power = 1;
    int torpedo_power = 8;

    UPirateShip(vec2i p, int c, u32 col);

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;

    bool isTarget(UActor* actor);
};

struct UPlayer : UShip
{
    bool is_aiming = false;
    bool is_aiming_railgun = false;

    int railgun_power = 1;
    int torpedo_power = 8;

    UPlayer(vec2i p) : UShip(UActorType::Player, p) {}

    void update(pcg32& rng) override;

    void render(TextBuffer& buffer, vec2i origin) override;
};

struct UTorpedo : UShip
{
    vec2i target_pos;
    u32 target = 0;
    u32 source = 0;

    int power;

    UTorpedo(vec2i p, int power) : UShip(UActorType::Torpedo, p), power(power) {}

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

struct UMilitaryStation : UActor
{
    int charge_time = 5;
    bool has_warned = false;

    UMilitaryStation(vec2i p);

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

    std::vector<UTorpedo*> torpedoes;

    pcg32 rng;
    int universe_ticks = 0;
    u32 next_actor = 1;

    bool has_spawned_alien = false;

    Universe();
    ~Universe();

    bool hasActor(vec2i p) { return actors.find(p).found; }

    bool isVisible(vec2i from, vec2i to);
    bool checkArea(vec2i pos, int radius);

    void move(UActor* a, vec2i d);
    void spawn(UActor* a);
    vec2i findEmpty(vec2i p);

    void update(vec2i origin);

    void render(TextBuffer& buffer, vec2i origin);
};
