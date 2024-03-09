
#include <cstdio>

#include "game.h"
#include "sound.h"
#include "vterm.h"
#include "universe.h"
#include "window.h"

void drawTitle(TextBuffer& buf)
{
    buf.write(vec2i(20, g_game.h - 12), "Salvage Scramble", 0xFFFFFFFF);
    buf.write(vec2i(20, g_game.h - 10), "Created for 7DRL 2024", 0xFFFFFFFF);
    buf.write(vec2i(20, g_game.h - 6), "Programming: TechnicalFowl", 0xFFFFFFFF);
    buf.write(vec2i(20, g_game.h - 5), "Art & Sound: Pepper", 0xFFFFFFFF);
}

void drawMenu(TextBuffer& buf)
{
    if (drawButton(&buf, vec2i(g_game.w + 5, g_game.h - 9), "Start Game", 0xFFFFFFFF))
    {
        g_game.state = GameState::Ingame;
        startGame();
    }
    if (drawButton(&buf, vec2i(g_game.w + 5, g_game.h - 8), "Settings", 0xFFFFFFFF))
    {
        g_game.state = GameState::Settings;
    }
    if (drawButton(&buf, vec2i(g_game.w + 5, g_game.h - 7), "Exit", 0xFFFFFFFF))
    {
        exit(0);
    }
}

void drawSlider(TextBuffer& buf, vec2i pos, int& value, const char* label)
{
    char slider[12]{ 0 };
    for (int i = 0; i < 11; ++i)
    {
        slider[i] = i == value ? FullChar : Border_Horizontal;
    }
    vec2f mouse = screen_mouse_pos();
    bool hovered = mouse.x >= pos.x / 2.0f && mouse.x < (pos.x + 11) / 2.0f && scalar::floori(mouse.y) == pos.y;
    u32 col = hovered ? 0xFF00FF00 : 0xFFFFFFFF;
    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        value = scalar::clamp(scalar::floori((mouse.x - pos.x / 2.0f) * 2.0f), 0, 10);
    }

    buf.write(pos, slider, 0xFFFFFFFF);
    buf.write(vec2i(pos.x + 13, pos.y), label, 0xFFFFFFFF);
}

void keySelector(TextBuffer& buf, vec2i pos, int& key, const char* label)
{
    static void* waiting_for = nullptr;

    vec2f mouse = screen_mouse_pos();
    bool hovered = mouse.x >= pos.x / 2.0f && mouse.x < (pos.x + 11llu + strlen(label)) / 2.0f && scalar::floori(mouse.y) == pos.y;
    u32 col = hovered ? 0xFF00FF00 : 0xFFFFFFFF;

    if (waiting_for == &key)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            waiting_for = nullptr;
        }
        int k = GetKeyPressed();
        if (k != 0)
        {
            waiting_for = nullptr;
            key = k;
        }
    }
    else if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        waiting_for = &key;
    }

    const char* keyname = waiting_for == &key ? "..." : GetKeyName(key);
    int padding = (8 - (int) strlen(keyname)) / 2;
    int padding2 = 8 - ((int) strlen(keyname) + padding);
    char pad[5]{ 0 };
    for (int i = 0; i < padding; ++i) pad[i] = ' ';
    char pad2[5]{ 0 };
    for (int i = 0; i < padding2; ++i) pad2[i] = ' ';

    sstring text; text.appendf("[%s%s%s] %s", pad, keyname, pad2, label);
    buf.write(vec2i(pos.x, pos.y), text.c_str(), col, LayerPriority_UI);
}

void drawSettings(TextBuffer& buf)
{
    drawSlider(buf, vec2i(g_game.w + 5, g_game.h - 10), g_game.music_volume, "Music Volume");
    drawSlider(buf, vec2i(g_game.w + 5, g_game.h - 9), g_game.sound_volume, "Effects Volume");

    keySelector(buf, vec2i(g_game.w + 5, g_game.h - 7), g_game.key_up, "Up");
    keySelector(buf, vec2i(g_game.w + 5, g_game.h - 6), g_game.key_down, "Down");
    keySelector(buf, vec2i(g_game.w + 5, g_game.h - 5), g_game.key_left, "Left");
    keySelector(buf, vec2i(g_game.w + 5, g_game.h - 4), g_game.key_right, "Right");
    keySelector(buf, vec2i(g_game.w + 5, g_game.h - 3), g_game.key_wait, "Wait");

    keySelector(buf, vec2i(g_game.w + 45, g_game.h - 10), g_game.key_open, "Open");
    keySelector(buf, vec2i(g_game.w + 45, g_game.h - 9), g_game.key_use, "Use-On");
    keySelector(buf, vec2i(g_game.w + 45, g_game.h - 8), g_game.key_pickup, "Pickup");
    keySelector(buf, vec2i(g_game.w + 45, g_game.h - 7), g_game.key_fire, "Fire");
    keySelector(buf, vec2i(g_game.w + 45, g_game.h - 6), g_game.key_railgun, "Railgun");
    keySelector(buf, vec2i(g_game.w + 45, g_game.h - 5), g_game.key_dock, "Dock");

}

void drawGameOver(TextBuffer& buf)
{
    buf.write(vec2i(g_game.w + 5, g_game.h - 10), "Game Over!", 0xFFFFFFFF);
    sstring line0; line0.appendf("Died to: %s", g_game.gameover_reason.c_str());
    buf.write(vec2i(g_game.w + 5, g_game.h - 8), line0.c_str(), 0xFFFFFFFF);

    if (drawButton(&buf, vec2i(g_game.w + 5, g_game.h - 5), "Start Game", 0xFFFFFFFF))
    {
        g_game.state = GameState::Ingame;
        startGame();
    }
    if (drawButton(&buf, vec2i(g_game.w + 5, g_game.h - 4), "Settings", 0xFFFFFFFF))
    {
        g_game.state = GameState::Settings;
    }
    if (drawButton(&buf, vec2i(g_game.w + 5, g_game.h - 3), "Exit", 0xFFFFFFFF))
    {
        exit(0);
    }

    buf.write(vec2i(g_game.w + 45, g_game.h - 10), "Turns: XXX", 0xFFFFFFFF);
    buf.write(vec2i(g_game.w + 45, g_game.h - 9), "Torpedoes Launched: XXX", 0xFFFFFFFF);
    buf.write(vec2i(g_game.w + 45, g_game.h - 8), "Railgun Fired: XXX", 0xFFFFFFFF);
    buf.write(vec2i(g_game.w + 45, g_game.h - 7), "Stations Visited: XXX", 0xFFFFFFFF);
    buf.write(vec2i(g_game.w + 45, g_game.h - 6), "Ships Killed: XXX", 0xFFFFFFFF);
    buf.write(vec2i(g_game.w + 45, g_game.h - 5), "Damage Taken: XXX", 0xFFFFFFFF);
}

int main(int argc, const char** argv)
{
    int w = 80;
    int h = 45;

    int scale = 16;

    initGame(w, h);
    window_open("7drl - Salvage Scramble", w * scale, h * scale);

    Image img = LoadImage("assets/ship_cover.png");
    if (!img.data)
    {
        while (!WindowShouldClose())
        {
            BeginDrawing();
            ClearBackground(BLACK);
            DrawText("Missing assets?", 100, h * scale / 2, 64, WHITE);
            EndDrawing();
        }
        return -1;
    }

    SetExitKey(0);

    g_game.state = GameState::MainMenu;
    TextBuffer* menuterm = new TextBuffer(w, h);

    g_game.universe = new Universe;

    int menu_frame = 0;
    vec2i menu_ship_pos = vec2i(0, 0);

    Texture menu_tex = LoadTextureFromImage(img);
    UnloadImage(img);

    initSounds();

    while (!WindowShouldClose())
    {
        updateSounds();

        BeginDrawing();

        g_window.width = GetScreenWidth();
        g_window.height = GetScreenHeight();

        g_game.w = GetScreenWidth() / 16;
        g_game.h = GetScreenHeight() / 16;

        if (g_game.state == GameState::Ingame)
        {
            updateGame();
        }

        if (g_game.state == GameState::Ingame || g_game.state == GameState::PauseMenu || g_game.state == GameState::GameOver)
        {
            int dw = g_window.width - g_game.w * 16;
            int dh = g_window.height - g_game.h * 16;

            ClearBackground(BLACK);
            BeginScissorMode(dw / 2, dh / 2, g_game.w * 16, g_game.h * 16);

            render_buffer(g_game.mapterm, g_window.map_zoom);
            if (g_game.state == GameState::Ingame)
                render_buffer(g_game.uiterm, 1.0f);

            if (g_game.state == GameState::PauseMenu || g_game.state == GameState::GameOver)
            {
                menuterm->clear(g_game.w, g_game.h);
                Rectangle dest{ dw / 2.0f, dh / 2.0f, g_game.w * 16.0f, g_game.h * 16.0f };
                Rectangle src{ 0.0f, 0.0f, (float)menu_tex.width, (float)menu_tex.height };
                DrawTexturePro(menu_tex, src, dest, Vector2(), 0.0f, WHITE);

                drawTitle(*menuterm);

                if (g_game.state == GameState::PauseMenu)
                {
                    drawSettings(*menuterm);

                    if (drawButton(menuterm, vec2i(g_game.w + 45, g_game.h - 3), "Return to Game", 0xFFFFFFFF))
                    {
                        g_game.state = GameState::Ingame;
                    }
                    if (drawButton(menuterm, vec2i(g_game.w + 45, g_game.h - 2), "Main Menu", 0xFFFFFFFF))
                    {
                        g_game.state = GameState::MainMenu;
                    }
                    if (drawButton(menuterm, vec2i(g_game.w + 45, g_game.h - 1), "Exit", 0xFFFFFFFF))
                    {
                        exit(0);
                    }
                }
                else
                {
                    drawGameOver(*menuterm);
                }

                render_buffer(menuterm, 1.0f);
            }

            EndScissorMode();
        }
        else if (g_game.state == GameState::MainMenu || g_game.state == GameState::Settings)
        {
            if (g_game.uplayer)
            {
                delete g_game.universe;
                g_game.universe = new Universe;
                g_game.uplayer = nullptr;
                g_game.ships.clear();
                g_game.current_level = nullptr;
                g_game.player_ship = nullptr;
            }

            menuterm->clear(g_game.w, g_game.h);
            int dw = g_window.width - g_game.w * 16;
            int dh = g_window.height - g_game.h * 16;
            ClearBackground(Color{0x20, 0x20, 0x20, 0xFF});

            g_game.mapterm->clear(g_game.w, g_game.h);
            menu_frame++;
            if ((menu_frame % 5) == 0)
            {
                menu_ship_pos += vec2i(1, 0);
                g_game.universe->update(menu_ship_pos);
            }
            g_game.universe->render(*g_game.mapterm, menu_ship_pos);
            render_buffer(g_game.mapterm, g_window.map_zoom);

            Rectangle dest{ dw / 2.0f, dh / 2.0f, g_game.w * 16.0f, g_game.h * 16.0f };
            Rectangle src{ 0.0f, 0.0f, (float) menu_tex.width, (float) menu_tex.height };
            DrawTexturePro(menu_tex, src, dest, Vector2(), 0.0f, WHITE);

            BeginScissorMode(dw / 2, dh / 2, g_game.w * 16, g_game.h * 16);

            drawTitle(*menuterm);

            if (g_game.state == GameState::MainMenu)
            {
                drawMenu(*menuterm);
            }
            else
            {
                drawSettings(*menuterm);

                if (drawButton(menuterm, vec2i(g_game.w + 45, g_game.h - 3), "Main Menu", 0xFFFFFFFF))
                {
                    g_game.state = GameState::MainMenu;
                }
            }

            render_buffer(menuterm, 1.0f);

            EndScissorMode();
        }

        EndDrawing();
    }

    return 0;
}