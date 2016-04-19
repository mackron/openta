// Public domain. See "unlicense" statement at the end of this file.

ta_game* ta_create_game(dr_cmdline cmdline)
{
    ta_game* pGame = calloc(1, sizeof(*pGame));
    if (pGame == NULL) {
        return NULL;
    }

    pGame->cmdline = cmdline;

    // File system. We want to set the working directory to the executable.
    char exedir[DRFS_MAX_PATH];
    if (!dr_get_executable_path(exedir, sizeof(exedir))) {
        goto on_error;
    }
    drpath_remove_file_name(exedir);

#ifdef _WIN32
    _chdir(exedir);
#else
    chdir(exedir)
#endif


    // We need to create a graphics context before we can create the game window.
    pGame->pGraphics = ta_create_graphics_context(pGame);
    if (pGame->pGraphics == NULL) {
        goto on_error;
    }

    // Create a show the window as soon as we can to make loading feel faster.
    pGame->pWindow = ta_graphics_create_window(pGame->pGraphics, "Total Annihilation", 640, 480, TA_WINDOW_FULLSCREEN | TA_WINDOW_CENTERED);
    if (pGame->pWindow == NULL) {
        goto on_error;
    }

    // Audio system.
    pGame->pAudioContext = draudio_create_context();
    if (pGame->pAudioContext == NULL) {
        goto on_error;
    }
    




    // Initialize the timer last so that the first frame has as accurate of a delta time as possible.
    pGame->pTimer = ta_create_timer();
    if (pGame->pTimer == NULL) {
        goto on_error;
    }

    return pGame;

on_error:
    if (pGame != NULL) {
        if (pGame->pWindow != NULL) {
            ta_delete_window(pGame->pWindow);
        }

        if (pGame->pGraphics != NULL) {
            ta_delete_graphics_context(pGame->pGraphics);
        }

        if (pGame->pAudioContext != NULL) {
            draudio_delete_context(pGame->pAudioContext);
        }

        if (pGame->pTimer != NULL) {
            ta_delete_timer(pGame->pTimer);
        }
    }

    return NULL;
}

void ta_delete_game(ta_game* pGame)
{
    if (pGame == NULL) {
        return;
    }

    ta_delete_window(pGame->pWindow);
    ta_delete_graphics_context(pGame->pGraphics);
    draudio_delete_context(pGame->pAudioContext);
    ta_delete_timer(pGame->pTimer);
    free(pGame);
}


int ta_game_run(ta_game* pGame)
{
    if (pGame == NULL) {
        return TA_ERROR_INVALID_ARGS;
    }

    return ta_main_loop(pGame);
}

void ta_do_frame(ta_game* pGame)
{
    assert(pGame != NULL);

    ta_game_step(pGame);
    ta_game_render(pGame);
}


void ta_game_step(ta_game* pGame)
{
    assert(pGame != NULL);

    // The first thing we need to do is figure out the delta time. We use high-resolution timing for this so we can have good accuracy
    // at high frame rates.
    const double dt = ta_tick_timer(pGame->pTimer);
}

void ta_game_render(ta_game* pGame)
{
    assert(pGame != NULL);

    ta_graphics_set_current_window(pGame->pGraphics, pGame->pWindow);
    {
    }
    ta_graphics_present(pGame->pGraphics, pGame->pWindow);
}
