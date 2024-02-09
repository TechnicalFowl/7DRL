
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

    window_open("7drl", w * scale, h * scale);

    while (frame_start())
    {
        updateGame();

        render_buffer(g_game.term);
        frame_end();
    }

    return 0;
}