// Copyright (C) 2018 David Reid. See included LICENSE file.

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


ta_result taEngineContextInit(int argc, char** argv, taStepProc onStep, void* pUserData, taEngineContext* pEngine)
{
    if (pEngine == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pEngine);
    pEngine->argc = argc;
    pEngine->argv = argv;
    pEngine->onStep = onStep;
    pEngine->pUserData = pUserData;

    ta_result result = TA_SUCCESS;

    //// File System ////
    pEngine->pFS = ta_create_file_system();
    if (pEngine->pFS == NULL) {
        result = TA_ERROR;
        goto on_error1;
    }


    //// Graphics ////

    // The palettes. The graphics system depends on these so they need to be loaded first.
    if (!ta_load_palette(pEngine->pFS, "palettes/PALETTE.PAL", pEngine->palette)) {
        result = TA_ERROR;
        goto on_error2;
    }

    if (!ta_load_palette(pEngine->pFS, "palettes/GUIPAL.PAL", pEngine->guipal)) {
        result = TA_ERROR;
        goto on_error2;
    }

    // Due to the way I'm doing a few things with the rendering, we want to use a specific entry in the palette to act as a
    // fully transparent value. For now I'll be using entry 240. Any non-transparent pixel that wants to use this entry will
    // be changed to use color 0. From what I can tell there should be no difference in visual appearance by doing this.
    pEngine->palette[TA_TRANSPARENT_COLOR] &= 0x00FFFFFF; // <-- Make the alpha component fully transparent.

    pEngine->pGraphics = ta_create_graphics_context(pEngine, pEngine->palette);
    if (pEngine->pGraphics == NULL) {
        result = TA_ERROR;
        goto on_error2;
    }



    //// Input ////
    result = ta_input_state_init(&pEngine->input);
    if (result != TA_SUCCESS) {
        goto on_error3;
    }



    //// GUI ////

    // There are a few required resources that are hard coded from what I can tell.
    result = ta_font_load(pEngine, "anims/hattfont12.GAF/Haettenschweiler (120)", &pEngine->font);
    if (result != TA_SUCCESS) {
        goto on_error4;
    }

    result = ta_font_load(pEngine, "anims/hattfont11.GAF/Haettenschweiler (120)", &pEngine->fontSmall);
    if (result != TA_SUCCESS) {
        goto on_error5;
    }


    return TA_SUCCESS;

//on_error6: ta_font_unload(&pEngine->fontSmall);
on_error5: ta_font_unload(&pEngine->font);
on_error4: ta_input_state_uninit(&pEngine->input);
on_error3: ta_delete_graphics_context(pEngine->pGraphics);
on_error2: ta_delete_file_system(pEngine->pFS);
on_error1:
    return result;
}

ta_result taEngineContextUninit(taEngineContext* pEngine)
{
    if (pEngine == NULL) return TA_INVALID_ARGS;

    ta_font_unload(&pEngine->fontSmall);
    ta_font_unload(&pEngine->font);
    ta_input_state_uninit(&pEngine->input);
    ta_delete_graphics_context(pEngine->pGraphics);
    ta_delete_file_system(pEngine->pFS);
    return TA_SUCCESS;
}






void ta_capture_mouse(ta_window* pWindow)
{
    ta_window_capture_mouse(pWindow);
}

void ta_release_mouse(taEngineContext* pEngine)
{
    (void)pEngine;
    ta_window_release_mouse();
}


//// Events from Window

void ta_on_window_size(taEngineContext* pEngine, unsigned int newWidth, unsigned int newHeight)
{
    assert(pEngine != NULL);
    ta_set_resolution(pEngine->pGraphics, newWidth, newHeight);
}

void ta_on_mouse_button_down(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    ta_input_state_on_mouse_button_down(&pEngine->input, button);
    //ta_capture_mouse(pEngine);
}

void ta_on_mouse_button_up(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    ta_input_state_on_mouse_button_up(&pEngine->input, button);

    //if (!ta_input_state_is_any_mouse_button_down(&pEngine->input)) {
    //    ta_release_mouse(pEngine);
    //}
}

void ta_on_mouse_button_dblclick(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)pEngine;
    (void)button;
    (void)posX;
    (void)posY;
    (void)stateFlags;
}

void ta_on_mouse_wheel(taEngineContext* pEngine, int delta, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)pEngine;
    (void)delta;
    (void)posX;
    (void)posY;
    (void)stateFlags;
}

void ta_on_mouse_move(taEngineContext* pEngine, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)stateFlags;

    ta_input_state_on_mouse_move(&pEngine->input, (float)posX, (float)posY);
}

void ta_on_mouse_enter(taEngineContext* pEngine)
{
    assert(pEngine != NULL);
    (void)pEngine;
}

void ta_on_mouse_leave(taEngineContext* pEngine)
{
    assert(pEngine != NULL);
    (void)pEngine;
}

void ta_on_key_down(taEngineContext* pEngine, ta_key key, unsigned int stateFlags)
{
    assert(pEngine != NULL);

    if (key < ta_countof(pEngine->input.keyState)) {
        if ((stateFlags & TA_KEY_STATE_AUTO_REPEATED) == 0) {
            ta_input_state_on_key_down(&pEngine->input, key);
        }
    }
}

void ta_on_key_up(taEngineContext* pEngine, ta_key key, unsigned int stateFlags)
{
    assert(pEngine != NULL);

    if (key < ta_countof(pEngine->input.keyState)) {
        ta_input_state_on_key_up(&pEngine->input, key);
    }
}

void ta_on_printable_key_down(taEngineContext* pEngine, uint32_t utf32, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)pEngine;
    (void)utf32;
    (void)stateFlags;
}
