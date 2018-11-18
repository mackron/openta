// Copyright (C) 2018 David Reid. See included LICENSE file.

// The main file containing the entry point. This is the only compiled file for the entire game.

#include "taGame/taGame.c"

int main(int argc, char** argv)
{
    // The window system needs to be initialized once, before creating the game context.
    taInitWindowSystem();

    taGame* pGame = taCreateGame(argc, argv);
    if (pGame == NULL) {
        return TA_ERROR_FAILED_TO_CREATE_GAME_CONTEXT;
    }

    int result = taGameRun(pGame);


    taDeleteGame(pGame);
    taUninitWindowSystem();

    return result;
}