#include "sound.h"

#include "game.h"

struct Sounds
{
    Sound effects[SoundEffectCount];
    Music music[2];
    int active_music = 0;

    int sound_volume = 10;
    int music_volume = 10;
};
Sounds g_sounds;

void initSounds()
{
    InitAudioDevice();

    g_sounds.effects[int(SoundEffect::AsteroidImpact)] = LoadSound("assets/asteroid_impact.wav");
    g_sounds.effects[int(SoundEffect::PDCFire)] = LoadSound("assets/pdc_fire.wav");
    g_sounds.effects[int(SoundEffect::RailgunFire)] = LoadSound("assets/railgun_fire.wav");
    g_sounds.effects[int(SoundEffect::RailgunImpact)] = LoadSound("assets/railgun_impact.wav");
    g_sounds.effects[int(SoundEffect::ReactorShutdown)] = LoadSound("assets/reactor_shutdown.wav");
    g_sounds.effects[int(SoundEffect::ReactorStartup)] = LoadSound("assets/reactor_start.wav");
    g_sounds.effects[int(SoundEffect::TorpedoImpact)] = LoadSound("assets/torpedo_impact.wav");
    g_sounds.effects[int(SoundEffect::TorpedoLaunch)] = LoadSound("assets/torpedo_launch.wav");

    g_sounds.music[0] = LoadMusicStream("assets/levelloopmaybe.wav");
    g_sounds.music[1] = LoadMusicStream("assets/levelloopmaybedrumver.wav");

    if (g_game.sound_volume != g_sounds.sound_volume)
    {
        g_sounds.sound_volume = g_game.sound_volume;
        for (int i = 0; i < SoundEffectCount; i++)
        {
            SetSoundVolume(g_sounds.effects[i], g_sounds.sound_volume / 10.0f);
        }
    }
    if (g_game.music_volume != g_sounds.music_volume)
    {
        g_sounds.music_volume = g_game.music_volume;
        for (int i = 0; i < 2; i++)
        {
            SetMusicVolume(g_sounds.music[i], (g_sounds.music_volume / 10.0f) * 0.4f);
        }
    }

    PlayMusicStream(g_sounds.music[0]);
}

void updateSounds()
{
    if (g_game.sound_volume != g_sounds.sound_volume)
    {
        g_sounds.sound_volume = g_game.sound_volume;
        for (int i = 0; i < SoundEffectCount; i++)
        {
            SetSoundVolume(g_sounds.effects[i], g_sounds.sound_volume / 10.0f);
        }
    }
    if (g_game.music_volume != g_sounds.music_volume)
    {
        g_sounds.music_volume = g_game.music_volume;
        for (int i = 0; i < 2; i++)
        {
            SetMusicVolume(g_sounds.music[i], (g_sounds.music_volume / 10.0f) * 0.4f);
        }
    }

    UpdateMusicStream(g_sounds.music[g_sounds.active_music]);
    float pct = GetMusicTimePlayed(g_sounds.music[g_sounds.active_music]) / GetMusicTimeLength(g_sounds.music[g_sounds.active_music]);
    if (pct >= 1.0f)
    {
        StopMusicStream(g_sounds.music[g_sounds.active_music]);
        g_sounds.active_music = (g_sounds.active_music + 1) % 2;
        PlayMusicStream(g_sounds.music[g_sounds.active_music]);
    }
}

void playSound(SoundEffect effect)
{
    PlaySound(g_sounds.effects[int(effect)]);
}
