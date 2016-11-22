// Copyright (C) 2016 David Reid. See included LICENSE file.

ta_bool32 ta_load_palette(ta_fs* pFS, const char* filePath, uint32_t* paletteOut)
{
    ta_file* pPaletteFile = ta_open_file(pFS, filePath, 0);
    if (pPaletteFile == NULL) {
        return TA_FALSE;
    }

    size_t bytesRead;
    if (!ta_read_file(pPaletteFile, paletteOut, 1024, &bytesRead) || bytesRead != 1024) {
        return TA_FALSE;
    }

    ta_close_file(pPaletteFile);


    // The palettes will include room for an alpha component, but it'll always be set to 0 (fully transparent). However, due
    // to the way I'm doing things in this project it's better for us to convert all of them to fully opaque.
    for (int i = 0; i < 256; ++i) {
        paletteOut[i] |= 0xFF000000;
    }

    return TA_TRUE;
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
    pGame->pWindow = ta_graphics_create_window(pGame->pGraphics, "Total Annihilation", 1280, 720, TA_WINDOW_FULLSCREEN | TA_WINDOW_CENTERED);
    if (pGame->pWindow == NULL) {
        goto on_error;
    }

    // Audio system.
    dra_context_create(&pGame->pAudioContext);
    if (pGame->pAudioContext == NULL) {
        goto on_error;
    }


    // Input.
    if (ta_input_state_init(&pGame->input) != TA_SUCCESS) {
        goto on_error;
    }


    // Properties.
    if (ta_property_manager_init(&pGame->properties) != TA_SUCCESS) {
        goto on_error;
    }

    // Hard coded properties. Some of these may be incorrect but we'll fix it up as we go.
    ta_set_property(pGame, "MAINMENU.GUI.BACKGROUND", "bitmaps/FrontendX.pcx");



    // GUI
    // ===
    if (ta_common_gui_load(pGame, &pGame->commonGUI) != TA_SUCCESS) {
        goto on_error;
    }

    // There are a few required resources that are hard coded from what I can tell.
    //ta_font_load(pGame, "fonts/HATT12.FNT", &pGame->font);
    ta_font_load(pGame, "anims/hattfont12.GAF/Haettenschweiler (120)", &pGame->font);

    




    // Texture GAFs. This is every GAF file contained in the "textures" directory. These are loaded in two passes. The first
    // pass counts the number of GAF files, and the second pass opens them.
    pGame->textureGAFCount = 0;
    ta_fs_iterator* iGAF = ta_fs_begin(pGame->pFS, "textures", TA_FALSE);
    while (ta_fs_next(iGAF)) {
        if (drpath_extension_equal(iGAF->fileInfo.relativePath, "gaf")) {
            pGame->textureGAFCount += 1;
        }
    }
    ta_fs_end(iGAF);

    pGame->ppTextureGAFs = (ta_gaf**)malloc(pGame->textureGAFCount * sizeof(*pGame->ppTextureGAFs));
    if (pGame->ppTextureGAFs == NULL) {
        goto on_error;  // Failed to load texture GAFs.
    }

    pGame->textureGAFCount = 0;
    iGAF = ta_fs_begin(pGame->pFS, "textures", TA_FALSE);
    while (ta_fs_next(iGAF)) {
        if (drpath_extension_equal(iGAF->fileInfo.relativePath, "gaf")) {
            pGame->ppTextureGAFs[pGame->textureGAFCount] = ta_open_gaf(pGame->pFS, iGAF->fileInfo.relativePath);
            pGame->textureGAFCount += 1;
        }
    }
    ta_fs_end(iGAF);


    // Main menu.
    if (ta_gui_load(pGame, "guis/MAINMENU.GUI", &pGame->mainMenu) != TA_SUCCESS) {
        goto on_error;
    }


    // Features.
    pGame->pFeatures = ta_create_features_library(pGame->pFS);
    if (pGame->pFeatures == NULL) {
        goto on_error;
    }



    dr_timer_init(&pGame->timer);


    // Switch to the main menu by default.
    ta_goto_screen(pGame, TA_SCREEN_MAIN_MENU);
    


    //ta_graphics_disable_vsync(pGame->pGraphics, pGame->pWindow);


    // TESTING
#if 1
    //pGame->pCurrentMap = ta_load_map(pGame, "The Pass");
    //pGame->pCurrentMap = ta_load_map(pGame, "Red Hot Lava");
    //pGame->pCurrentMap = ta_load_map(pGame, "Test0");
    pGame->pCurrentMap = ta_load_map(pGame, "AC01");    // <-- Includes 3DO features.
    //pGame->pCurrentMap = ta_load_map(pGame, "AC06");    // <-- Good profiling test.
    //pGame->pCurrentMap = ta_load_map(pGame, "AC20");
    //pGame->pCurrentMap = ta_load_map(pGame, "CC25");
#endif


    return pGame;

on_error:
    if (pGame != NULL) {
        if (pGame->pCurrentMap != NULL) ta_unload_map(pGame->pCurrentMap);
        if (pGame->pWindow != NULL) ta_delete_window(pGame->pWindow);
        if (pGame->pGraphics != NULL) ta_delete_graphics_context(pGame->pGraphics);
        if (pGame->pAudioContext != NULL) dra_context_delete(pGame->pAudioContext);
        if (pGame->pFS != NULL) ta_delete_file_system(pGame->pFS);
    }

    return NULL;
}

void ta_delete_game(ta_game* pGame)
{
    if (pGame == NULL) {
        return;
    }

    ta_delete_window(pGame->pWindow);
    ta_property_manager_uninit(&pGame->properties);
    ta_input_state_uninit(&pGame->input);
    ta_delete_graphics_context(pGame->pGraphics);
    dra_context_delete(pGame->pAudioContext);
    ta_delete_file_system(pGame->pFS);
    free(pGame);
}


ta_result ta_set_property(ta_game* pGame, const char* key, const char* value)
{
    return ta_property_manager_set(&pGame->properties, key, value);
}

const char* ta_get_property(ta_game* pGame, const char* key)
{
    return ta_property_manager_get(&pGame->properties, key);
}

const char* ta_get_propertyf(ta_game* pGame, const char* key, ...)
{
    const char* value = NULL;

    va_list args;
    va_start(args, key);
    {
        char* formattedKey = ta_make_stringv(key, args);
        if (formattedKey != NULL) {
            value = ta_get_property(pGame, formattedKey);
            ta_free_string(formattedKey);
        }
    }
    va_end(args);

    return value;
}


int ta_game_run(ta_game* pGame)
{
    if (pGame == NULL) {
        return TA_ERROR_INVALID_ARGS;
    }

    return ta_main_loop(pGame);
}


void ta_step__in_game(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_IN_GAME);
    (void)dt;

    if (ta_is_mouse_button_down(pGame, TA_MOUSE_BUTTON_MIDDLE)) {
        ta_translate_camera(pGame->pGraphics, -(int)pGame->input.mouseOffsetX, -(int)pGame->input.mouseOffsetY);
    }

    if (pGame->pCurrentMap) {
        ta_draw_map(pGame->pGraphics, pGame->pCurrentMap);
    }
}

void ta_step__main_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_MAIN_MENU);
    (void)dt;

    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->mainMenu);
    //ta_draw_text(pGame->pGraphics, &pGame->font, 255, 1, 32, 32, "Hello, World!");
    ta_draw_textf(pGame->pGraphics, &pGame->font, 255, 1, 32, 32, "Mouse Position: %d %d", (int)pGame->input.mousePosX, (int)pGame->input.mousePosY);
}

void ta_step(ta_game* pGame)
{
    assert(pGame != NULL);

    const double dt = dr_timer_tick(&pGame->timer);
    ta_graphics_set_current_window(pGame->pGraphics, pGame->pWindow);
    {
        // The first thing we need to do is figure out the delta time. We use high-resolution timing for this so we can have good accuracy
        // at high frame rates.
        switch (pGame->screen)
        {
            case TA_SCREEN_IN_GAME:
            {
                ta_step__in_game(pGame, dt);
            } break;

            case TA_SCREEN_MAIN_MENU:
            {
                ta_step__main_menu(pGame, dt);
            } break;

            default: break;
        }

        // Reset transient input state last.
        ta_input_state_reset_transient_state(&pGame->input);
    }
    ta_graphics_present(pGame->pGraphics, pGame->pWindow);
}


void ta_goto_screen(ta_game* pGame, ta_uint32 newScreenType)
{
    if (pGame == NULL) return;
    pGame->screen = newScreenType;
}


ta_bool32 ta_is_mouse_button_down(ta_game* pGame, ta_uint32 button)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_is_mouse_button_down(&pGame->input, button);
}

ta_bool32 ta_was_mouse_button_pressed(ta_game* pGame, ta_uint32 button)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_mouse_button_pressed(&pGame->input, button);
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


ta_texture* ta_load_image(ta_game* pGame, const char* filePath)
{
    if (pGame == NULL || filePath == NULL) return NULL;
    
    if (drpath_extension_equal(filePath, "pcx")) {
        ta_file* pFile = ta_open_file(pGame->pFS, filePath, 0);
        if (pFile == NULL) {
            return NULL;    // File not found.
        }

        int width;
        int height;
        dr_uint8* pImageData = drpcx_load_memory(pFile->pFileData, pFile->sizeInBytes, TA_FALSE, &width, &height, NULL, 4);
        if (pImageData == NULL) {
            return NULL;    // Not a valid PCX file.
        }

        ta_texture* pTexture = ta_create_texture(pGame->pGraphics, (unsigned int)width, (unsigned int)height, 4, pImageData);
        if (pTexture == NULL) {
            drpcx_free(pImageData);
            return NULL;    // Failed to create texture.
        }

        drpcx_free(pImageData);
        return pTexture;
    }

    // Failed to open file.
    return NULL;
}


//// GUI ////

ta_texture* ta_get_gui_button_texture(ta_game* pGame, ta_uint32 width, ta_uint32 height, ta_uint32 state, ta_subtexture_metrics* pMetrics)
{
    if (pMetrics) ta_zero_object(pMetrics);
    if (pGame == NULL) return NULL;

    for (int i = 0; i < ta_countof(pGame->commonGUI.buttons); ++i) {
        if (pGame->commonGUI.buttons[i].sizeX == width && pGame->commonGUI.buttons[i].sizeY == height) {
            ta_uint32 subtextureIndex = pGame->commonGUI.buttons[i].frameIndex;
            if (state == TA_GUI_BUTTON_STATE_PRESSED) {
                subtextureIndex += 1;
            } else if (state == TA_GUI_BUTTON_STATE_DISABLED) {
                subtextureIndex += 2;
            }

            //if (pMetrics != NULL) *pMetrics = pGame->commonGUI.pSubtextures[subtextureIndex].metrics;
            //return pGame->commonGUI.ppTextures[pGame->commonGUI.pSubtextures[subtextureIndex].textureIndex];

            ta_gaf_texture_group_frame* pFrame = pGame->commonGUI.textureGroup.pFrames + subtextureIndex;
            if (pMetrics != NULL) {
                pMetrics->width       = pFrame->sizeX;
                pMetrics->height      = pFrame->sizeY;
                pMetrics->texturePosX = pFrame->atlasPosX;
                pMetrics->texturePosY = pFrame->atlasPosY;
            }

            return pGame->commonGUI.textureGroup.ppAtlases[pFrame->atlasIndex];
        }
    }

    return NULL;
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

    ta_input_state_on_mouse_button_down(&pGame->input, button);
    ta_capture_mouse(pGame);

#if 0
    // TODO: Properly handle mouse capture with respect to all mouse buttons.
    if (button == TA_MOUSE_BUTTON_MIDDLE) {
        pGame->isMMBDown = TA_TRUE;
        pGame->mouseDownPosX = posX;
        pGame->mouseDownPosY = posY;
    }
#endif
}

void ta_on_mouse_button_up(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    ta_input_state_on_mouse_button_up(&pGame->input, button);

    if (!ta_input_state_is_any_mouse_button_down(&pGame->input)) {
        ta_release_mouse(pGame);
    }

#if 0
    // TODO: Properly handle mouse capture with respect to all mouse buttons.
    if (button == TA_MOUSE_BUTTON_MIDDLE) {
        pGame->isMMBDown = TA_FALSE;
    }
#endif
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
    (void)stateFlags;

    ta_input_state_on_mouse_move(&pGame->input, (float)posX, (float)posY);

#if 0
    if (pGame->isMMBDown) {
        ta_translate_camera(pGame->pGraphics, pGame->mouseDownPosX - posX, pGame->mouseDownPosY - posY);
        pGame->mouseDownPosX = posX;
        pGame->mouseDownPosY = posY;
    }
#endif
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
