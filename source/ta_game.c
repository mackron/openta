// Public domain. See "unlicense" statement at the end of this file.

bool ta_load_palette(ta_fs* pFS, const char* filePath, uint32_t* paletteOut)
{
    ta_file* pPaletteFile = ta_open_file(pFS, filePath);
    if (pPaletteFile == NULL) {
        return false;
    }

    size_t bytesRead;
    if (!ta_read_file(pPaletteFile, paletteOut, 1024, &bytesRead) || bytesRead != 1024) {
        return false;
    }

    ta_close_file(pPaletteFile);


    // The palettes will include room for an alpha component, but it'll always be set to 0 (fully transparent). However, due
    // to the way I'm doing things in this project it's better for us to convert all of them to fully opaque.
    for (int i = 0; i < 256; ++i) {
        paletteOut[i] |= 0xFF000000;
    }

    return true;
}

bool ta_load_features(ta_game* pGame, const char* directoryPath)
{
    ta_fs_iterator* pIter = ta_fs_begin(pGame->pFS, directoryPath, true);
    while (ta_fs_next(pIter)) {
        if (!pIter->fileInfo.isDirectory) {
            printf("FILE: %s/%s\n", pIter->fileInfo.archiveRelativePath, pIter->fileInfo.relativePath);
        }
    }
    ta_fs_end(pIter);

    return true;
}

ta_game* ta_create_game(dr_cmdline cmdline)
{
    ta_game* pGame = calloc(1, sizeof(*pGame));
    if (pGame == NULL) {
        return NULL;
    }

    pGame->cmdline = cmdline;

    pGame->pFS = ta_create_file_system();
    if (pGame->pFS == NULL) {
        goto on_error;
    }



    // The palettes. The graphics system depends on these so they need to be loaded first.
    if (!ta_load_palette(pGame->pFS, "palettes/PALETTE.PAL", pGame->palette)) {
        goto on_error;
    }

    if (!ta_load_palette(pGame->pFS, "palettes/GUIPAL.PAL", pGame->guipal)) {
        goto on_error;
    }

    // Due to the way I'm doing a few things with the rendering, we want to use a specific entry in the palette to act as a
    // fully transparent value. For now I'll be using entry 240. Any non-transparent pixel that wants to use this entry will
    // be changed to use color 0. From what I can tell there should be no difference in visual appearance by doing this.
    pGame->palette[TA_TRANSPARENT_COLOR] &= 0x00FFFFFF; // <-- Make the alpha component fully transparent.



    // We need to create a graphics context before we can create the game window.
    pGame->pGraphics = ta_create_graphics_context(pGame, pGame->palette);
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


    // Features.
    if (!ta_load_features(pGame, "features")) {
        goto on_error;
    }



    // Initialize the timer last so that the first frame has as accurate of a delta time as possible.
    pGame->pTimer = ta_create_timer();
    if (pGame->pTimer == NULL) {
        goto on_error;
    }



    // TESTING
#if 0
    ta_hpi_archive* pHPI = ta_open_hpi_from_file("rev31.gp3");
    ta_hpi_file* pFile = ta_hpi_open_file(pHPI, "gamedata/WEAPONS.TDF");

    char* fileData = malloc(pFile->sizeInBytes + 1);
    ta_hpi_read(pFile, fileData, pFile->sizeInBytes, NULL);
    fileData[pFile->sizeInBytes] = '\0';

    ta_config_obj* pConfig = ta_parse_config(fileData);
#endif

#if 0
    ta_hpi_archive* pHPI = ta_open_hpi_from_file("totala1.hpi");
    ta_hpi_file* pFile = ta_hpi_open_file(pHPI, "anims/Archipelago.GAF");
    ta_gaf* pGAF = ta_load_gaf_from_file(pFile, pGame->pGraphics, pGame->palette);

    ta_gaf_entry_frame* pFrame = &pGAF->pEntries[0].pFrames[0];
    //ta_gaf_entry* pEntry = ta_gaf_get_entry_by_name(pGAF, "Frond01CrispRec");
    //ta_gaf_entry_frame* pFrame = &pEntry->pFrames[0].pSubframes[1];
    //ta_gaf_entry_frame* pFrame = &pEntry->pFrames[4];
    //pGame->pTexture = ta_create_texture(pGame->pGraphics, pFrame->width, pFrame->height, 1, pFrame->pImageData);
    //ta_unload_gaf(pGAF);

    pGame->pFrame = &pGAF->pEntries[0].pFrames[0];
    pGame->pTexture = pGAF->pTextureAtlases[pGame->pFrame->atlasIndex];
#endif

#if 1
    //ta_hpi_archive* pHPI = ta_open_hpi_from_file("totala2.hpi");
    //ta_hpi_file* pFile = ta_hpi_open_file(pHPI, "maps/The Pass.tnt");

    ta_tnt* pTNT = ta_load_tnt_from_file(pGame->pFS, "maps/The Pass.tnt", pGame->pGraphics);
    pGame->pTexture = pTNT->pMinimapTexture;
    pGame->pTNT = pTNT;
#endif


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

        if (pGame->pFS != NULL) {
            ta_delete_file_system(pGame->pFS);
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
    ta_delete_file_system(pGame->pFS);
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
        //ta_draw_texture(pGame->pTexture, true);
        
        //if (pGame->pFrame) {
        //    ta_draw_subtexture(pGame->pTexture, pGame->pFrame, pGame->pFrame->atlasPosX, pGame->pFrame->atlasPosY, pGame->pFrame->width, pGame->pFrame->height);
        //}

        if (pGame->pTNT) {
            ta_draw_terrain(pGame->pGraphics, pGame->pTNT);

            //ta_draw_subtexture(pGame->pTexture, false, 0, 0, pGame->pTNT->minimapWidth, pGame->pTNT->minimapHeight);
            //ta_draw_texture(pGame->pTNT->ppTextures[1], false);
/*
            for (uint32_t iChunk = 0; iChunk < pGame->pTNT->chunkCountX*pGame->pTNT->chunkCountY; ++iChunk) {
                ta_tnt_tile_chunk* pChunk = &pGame->pTNT->pChunks[iChunk];
                if (pChunk->subchunkCount > 0) {    // ?? Wouldn't have thought this would ever be true, but it is.
                    for (uint32_t iSubchunk = 0; iSubchunk < pChunk->subchunkCount; ++iSubchunk) {
                        ta_draw_tnt_mesh(pChunk->pSubchunks[iSubchunk].pTexture, pChunk->pSubchunks[iSubchunk].pMesh);
                    }
                }
            }
            */
            //ta_draw_tnt_mesh(pGame->pTNT->pChunks[1].pSubchunks[0].pTexture, pGame->pTNT->pChunks[1].pSubchunks[0].pMesh);
        }
    }
    ta_graphics_present(pGame->pGraphics, pGame->pWindow);
}


void ta_capture_mouse(ta_game* pGame)
{
    ta_window_capture_mouse(pGame->pWindow);
}

void ta_release_mouse(ta_game* pGame)
{
    (void)pGame;
    ta_window_release_mouse();
}



//// Events from Window

void ta_on_window_size(ta_game* pGame, unsigned int newWidth, unsigned int newHeight)
{
    assert(pGame != NULL);
    ta_set_resolution(pGame->pGraphics, newWidth, newHeight);
}

void ta_on_mouse_button_down(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    // TODO: Properly handle mouse capture with respect to all mouse buttons.

    if (button == TA_MOUSE_BUTTON_MIDDLE) {
        pGame->isMMBDown = true;
        pGame->mouseDownPosX = posX;
        pGame->mouseDownPosY = posY;
        ta_capture_mouse(pGame);
    }
}

void ta_on_mouse_button_up(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    // TODO: Properly handle mouse capture with respect to all mouse buttons.

    if (button == TA_MOUSE_BUTTON_MIDDLE) {
        pGame->isMMBDown = false;
        ta_release_mouse(pGame);
    }
}

void ta_on_mouse_button_dblclick(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)button;
    (void)posX;
    (void)posY;
    (void)stateFlags;
}

void ta_on_mouse_wheel(ta_game* pGame, int delta, int posX, int posY, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)delta;
    (void)posX;
    (void)posY;
    (void)stateFlags;
}

void ta_on_mouse_move(ta_game* pGame, int posX, int posY, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)posX;
    (void)posY;
    (void)stateFlags;

    if (pGame->isMMBDown) {
        ta_translate_camera(pGame->pGraphics, pGame->mouseDownPosX - posX, pGame->mouseDownPosY - posY);
        pGame->mouseDownPosX = posX;
        pGame->mouseDownPosY = posY;
    }
}

void ta_on_mouse_enter(ta_game* pGame)
{
    assert(pGame != NULL);
    (void)pGame;
}

void ta_on_mouse_leave(ta_game* pGame)
{
    assert(pGame != NULL);
    (void)pGame;
}

void ta_on_key_down(ta_game* pGame, ta_key key, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)key;
    (void)stateFlags;
}

void ta_on_key_up(ta_game* pGame, ta_key key, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)key;
    (void)stateFlags;
}

void ta_on_printable_key_down(ta_game* pGame, uint32_t utf32, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)utf32;
    (void)stateFlags;
}
