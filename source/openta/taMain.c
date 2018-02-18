// Copyright (C) 2018 David Reid. See included LICENSE file.

// The main file containing the entry point. This is the only compiled file for the entire game.

#include "taGame/taGame.c"

int main(int argc, char** argv)
{
    // The window system needs to be initialized once, before creating the game context.
    ta_init_window_system();

    ta_game* pGame = ta_create_game(argc, argv);
    if (pGame == NULL) {
        return TA_ERROR_FAILED_TO_CREATE_GAME_CONTEXT;
    }

    int result = ta_game_run(pGame);


    ta_delete_game(pGame);
    ta_uninit_window_system();

    return result;
}