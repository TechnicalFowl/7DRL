#include "game.h"

#include "vterm.h"

Game g_game;

void initGame(int w, int h)
{
    g_game.term = new TextBuffer(w, h);
}
