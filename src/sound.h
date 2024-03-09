#pragma once

enum class SoundEffect
{
    AsteroidImpact,
    PDCFire,
    RailgunFire,
    RailgunImpact,
    ReactorShutdown,
    ReactorStartup,
    TorpedoImpact,
    TorpedoLaunch,

    __COUNT,
};
constexpr int SoundEffectCount = int(SoundEffect::__COUNT);

void initSounds();
void updateSounds();

void playSound(SoundEffect effect);
