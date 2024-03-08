
#include <cstdio>

#include "game.h"
#include "vterm.h"
#include "window.h"

int main(int argc, const char** argv)
{
    int w = 80;
    int h = 45;

    int scale = 16;

    initGame(w, h);

    window_open("7drl - Salvage Scramble", w * scale, h * scale);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        g_window.width = GetScreenWidth();
        g_window.height = GetScreenHeight();

        updateGame();

        int dw = g_window.width - g_game.w * 16;
        int dh = g_window.height - g_game.h * 16;

        BeginScissorMode(dw / 2, dh / 2, g_game.w * 16, g_game.h * 16);

        render_buffer(g_game.mapterm, g_window.map_zoom);
        render_buffer(g_game.uiterm, 1.0f);

        EndScissorMode();

        EndDrawing();
    }

    return 0;
}