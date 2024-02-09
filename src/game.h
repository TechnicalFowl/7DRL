#pragma once

struct TextBuffer;

struct Game
{
    int w, h;

    TextBuffer* term;
};
extern Game g_game;

void initGame(int w, int h);
