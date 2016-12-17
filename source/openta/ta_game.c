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

int ta_qsort_cb_map(const void* a, const void* b)
{
    const ta_config_obj** ppOTA_0 = (const ta_config_obj**)a;
    const ta_config_obj** ppOTA_1 = (const ta_config_obj**)b;

    const char* name0 = ta_config_get_string(*ppOTA_0, "GlobalHeader/missionname");
    const char* name1 = ta_config_get_string(*ppOTA_1, "GlobalHeader/missionname");

    return _stricmp(name0, name1);
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

    ta_property_manager_load_settings(&pGame->properties);


    // GUI
    // ===
    if (ta_common_gui_load(pGame, &pGame->commonGUI) != TA_SUCCESS) {
        goto on_error;
    }

    // There are a few required resources that are hard coded from what I can tell.
    ta_font_load(pGame, "anims/hattfont12.GAF/Haettenschweiler (120)", &pGame->font);
    ta_font_load(pGame, "anims/hattfont11.GAF/Haettenschweiler (120)", &pGame->fontSmall);

    




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


    // Menus.
    ta_set_property(pGame, "MAINMENU.GUI.BACKGROUND", "bitmaps/FrontendX.pcx");
    if (ta_gui_load(pGame, "guis/MAINMENU.GUI", &pGame->mainMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SINGLE.GUI.BACKGROUND", "bitmaps/SINGLEBG.PCX");
    if (ta_gui_load(pGame, "guis/SINGLE.GUI", &pGame->spMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SELPROV.GUI.BACKGROUND", "bitmaps/selconnect2.pcx");
    if (ta_gui_load(pGame, "guis/SELPROV.GUI", &pGame->mpMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "STARTOPT.GUI.BACKGROUND", "bitmaps/options4x.pcx");
    if (ta_gui_load(pGame, "guis/STARTOPT.GUI", &pGame->optionsMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SKIRMISH.GUI.BACKGROUND", "bitmaps/Skirmsetup4x.pcx");
    if (ta_gui_load(pGame, "guis/SKIRMISH.GUI", &pGame->skirmishMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "NEWGAME.GUI.BACKGROUND", "bitmaps/playanygame4.pcx");
    if (ta_gui_load(pGame, "guis/NEWGAME.GUI", &pGame->campaignMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SELMAP.GUI.BACKGROUND", "bitmaps/DSelectmap2.pcx");
    if (ta_gui_load(pGame, "guis/SELMAP.GUI", &pGame->selectMapDialog) != TA_SUCCESS) {
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
    
    // TODO:
    // - Only load these when needed, i.e. when the Select Map dialog or campaign menu is opened.
    // - Save memory by converting OTA data to a struct rather than just holding a pointer to the ta_config_obj.
    //
    // Grab the maps for skirmish and multiplayer.
    ta_fs_iterator* pIter = ta_fs_begin(pGame->pFS, "maps", TA_FALSE);
    do
    {
        // From what I can tell, it looks like skirmish/mp maps are determined by the "type" property of the first
        // schema in the OTA file.
        if (drpath_extension_equal(pIter->fileInfo.relativePath, "ota")) {
            ta_config_obj* pOTA = ta_parse_config_from_file(pGame->pFS, pIter->fileInfo.relativePath);
            if (pOTA != NULL) {
                const char* type = ta_config_get_string(pOTA, "GlobalHeader/Schema 0/Type");
                if (type != NULL && _stricmp(type, "Network 1") == 0) {
                    stb_sb_push(pGame->ppMPMaps, pOTA);
                    printf("MP Map: %s\n", pIter->fileInfo.relativePath);
                } else {
                    stb_sb_push(pGame->ppSPMaps, pOTA);
                    printf("SP Map: %s\n", pIter->fileInfo.relativePath);
                }
            }
        }
    } while (ta_fs_next(pIter));
    ta_fs_end(pIter);

    // Sort alphabetically.
    qsort(pGame->ppMPMaps, stb_sb_count(pGame->ppMPMaps), sizeof(*pGame->ppMPMaps), ta_qsort_cb_map);
    qsort(pGame->ppSPMaps, stb_sb_count(pGame->ppSPMaps), sizeof(*pGame->ppSPMaps), ta_qsort_cb_map);

    ta_uint32 iMapListGadget;
    if (ta_gui_find_gadget_by_name(&pGame->selectMapDialog, "MAPNAMES", &iMapListGadget)) {
        const char** ppMPMapNames = (const char**)malloc(stb_sb_count(pGame->ppMPMaps) * sizeof(*ppMPMapNames));
        for (int i = 0; i < stb_sb_count(pGame->ppMPMaps); ++i) {
            ppMPMapNames[i] = ta_config_get_string(pGame->ppMPMaps[i], "GlobalHeader/missionname");
        }

        ta_gui_set_listbox_items(&pGame->selectMapDialog.pGadgets[iMapListGadget], ppMPMapNames, stb_sb_count(pGame->ppMPMaps));
    }

    


    // TESTING
#if 1
    //pGame->pCurrentMap = ta_load_map(pGame, "The Pass");
    //pGame->pCurrentMap = ta_load_map(pGame, "Red Hot Lava");
    //pGame->pCurrentMap = ta_load_map(pGame, "Test0");
    //pGame->pCurrentMap = ta_load_map(pGame, "AC01");    // <-- Includes 3DO features.
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

ta_result ta_set_property_int(ta_game* pGame, const char* key, ta_int32 value)
{
    char valueStr[256];
    snprintf(valueStr, sizeof(valueStr), "%d", value);
    return ta_set_property(pGame, key, valueStr);
}

ta_result ta_set_property_float(ta_game* pGame, const char* key, float value)
{
    char valueStr[256];
    snprintf(valueStr, sizeof(valueStr), "%f", value);
    return ta_set_property(pGame, key, valueStr);
}

ta_result ta_set_property_bool(ta_game* pGame, const char* key, ta_bool32 value)
{
    return ta_set_property(pGame, key, value ? "1" : "0");
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

ta_int32 ta_get_property_int(ta_game* pGame, const char* key)
{
    const char* valueStr = ta_get_property(pGame, key);
    if (valueStr == NULL) {
        return 0;
    }

    return atoi(valueStr);
}

float ta_get_property_float(ta_game* pGame, const char* key)
{
    const char* valueStr = ta_get_property(pGame, key);
    if (valueStr == NULL) {
        return 0;
    }

    return (float)atof(valueStr);
}

ta_bool32 ta_get_property_bool(ta_game* pGame, const char* key)
{
    const char* valueStr = ta_get_property(pGame, key);
    if (valueStr == NULL) {
        return TA_FALSE;
    }

    if ((key[0] == '0' && key[1] == '\0') || _stricmp(key, "false") == 0) {
        return TA_FALSE;
    }

    return TA_TRUE;
}


int ta_game_run(ta_game* pGame)
{
    if (pGame == NULL) {
        return TA_ERROR_INVALID_ARGS;
    }

    return ta_main_loop(pGame);
}

void ta_close(ta_game* pGame)
{
    if (pGame == NULL) return;

#ifdef _WIN32
    PostQuitMessage(0);
#endif

    pGame->isClosing = TA_TRUE;
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


typedef struct
{
    ta_uint32 type;
    ta_gui_gadget* pGadget;  // The gadget this event relates to.
} ta_gui_input_event;

dr_bool32 ta_handle_gui_input(ta_game* pGame, ta_gui* pGUI, ta_gui_input_event* pEvent)
{
    if (pGame == NULL || pGUI == NULL || pEvent == NULL) return TA_FALSE;
    ta_zero_object(pEvent);

    // Sanity check.
    if (pGUI->gadgetCount == 0) return TA_FALSE;

    // An so here's the good old GUI input handling code. No matter how many times I do this it never get's better :(

    // Note: We don't always want to return an event, and there may be times where all we do is change the state of relevant
    //       gadgets. We return true if an event is returned, false if no event was returned.

    // Keyboard
    // ========
    //
    // Keys to handle:
    // - Enter
    // - Escape
    // - Shortcuts for buttons
    ta_gui_gadget* pRootGadget = &pGUI->pGadgets[0];
    if (ta_was_key_pressed(pGame, TA_KEY_ENTER)) {
        // When the enter key is pressed we want to prioritize crdefault. It that is not set we fall back to the focused gadget.
        if (!ta_is_string_null_or_empty(pRootGadget->root.crdefault)) {
            ta_uint32 iGadget;
            ta_bool32 foundGadget = ta_gui_find_gadget_by_name(pGUI, pRootGadget->root.crdefault, &iGadget);
            if (foundGadget) {
                pEvent->type    = TA_GUI_EVENT_TYPE_BUTTON_PRESSED;
                pEvent->pGadget = &pGUI->pGadgets[iGadget];
                return TA_TRUE;
            }
        } else {
            // crdefault is not set, so try the focused element.
            ta_uint32 iGadget;
            ta_bool32 foundGadget = ta_gui_get_focused_gadget(pGUI, &iGadget);
            if (foundGadget) {
                pEvent->type    = TA_GUI_EVENT_TYPE_BUTTON_PRESSED;
                pEvent->pGadget = &pGUI->pGadgets[iGadget];
                return TA_TRUE;
            }
        }
    } else if (ta_was_key_pressed(pGame, TA_KEY_SPACE)) {
        ta_uint32 iGadget;
        ta_bool32 foundGadget = ta_gui_get_focused_gadget(pGUI, &iGadget);
        if (foundGadget) {
            pEvent->type    = TA_GUI_EVENT_TYPE_BUTTON_PRESSED;
            pEvent->pGadget = &pGUI->pGadgets[iGadget];
            return TA_TRUE;
        }
    } else if (ta_was_key_pressed(pGame, TA_KEY_ESCAPE)) {
        if (!ta_is_string_null_or_empty(pRootGadget->root.escdefault)) {
            ta_uint32 iGadget;
            ta_bool32 foundGadget = ta_gui_find_gadget_by_name(pGUI, pRootGadget->root.escdefault, &iGadget);
            if (foundGadget) {
                pEvent->type    = TA_GUI_EVENT_TYPE_BUTTON_PRESSED;
                pEvent->pGadget = &pGUI->pGadgets[iGadget];
                return TA_TRUE;
            }
        }
    } else if (ta_was_key_pressed(pGame, TA_KEY_ARROW_LEFT)) {
        ta_gui_focus_prev_gadget(pGUI);
    } else if (ta_was_key_pressed(pGame, TA_KEY_ARROW_RIGHT)) {
        ta_gui_focus_next_gadget(pGUI);
    } else if (ta_was_key_pressed(pGame, TA_KEY_ARROW_UP)) {
        ta_uint32 iFocusedGadget;
        if (ta_gui_get_focused_gadget(pGUI, &iFocusedGadget) && pGUI->pGadgets[iFocusedGadget].id == TA_GUI_GADGET_TYPE_LISTBOX) {
            ta_uint32 iSelectedItem = pGUI->pGadgets[iFocusedGadget].listbox.iSelectedItem;
            if (iSelectedItem == (ta_uint32)-1 || iSelectedItem == 0) {
                iSelectedItem = pGUI->pGadgets[iFocusedGadget].listbox.itemCount - 1;
            } else {
                iSelectedItem -= 1;
            }

            pGUI->pGadgets[iFocusedGadget].listbox.iSelectedItem = iSelectedItem;
        } else {
            ta_gui_focus_prev_gadget(pGUI);
        }
    } else if (ta_was_key_pressed(pGame, TA_KEY_ARROW_DOWN)) {
        ta_uint32 iFocusedGadget;
        if (ta_gui_get_focused_gadget(pGUI, &iFocusedGadget) && pGUI->pGadgets[iFocusedGadget].id == TA_GUI_GADGET_TYPE_LISTBOX) {
            ta_uint32 iSelectedItem = pGUI->pGadgets[iFocusedGadget].listbox.iSelectedItem;
            if (iSelectedItem == (ta_uint32)-1 || iSelectedItem == pGUI->pGadgets[iFocusedGadget].listbox.itemCount-1) {
                iSelectedItem = 0;
            } else {
                iSelectedItem += 1;
            }

            pGUI->pGadgets[iFocusedGadget].listbox.iSelectedItem = iSelectedItem;
        } else {
            ta_gui_focus_next_gadget(pGUI);
        }
    }  else if (ta_was_key_pressed(pGame, TA_KEY_TAB)) {
        if (ta_is_key_down(pGame, TA_KEY_SHIFT)) {
            ta_gui_focus_prev_gadget(pGUI);
        } else {
            ta_gui_focus_next_gadget(pGUI);
        }
    } else {
        // Try shortcut keys. For this we just iterate over each button and check if it's shortcut key was pressed.
        for (ta_uint32 iGadget = 1; iGadget < pGUI->gadgetCount; ++iGadget) {
            ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget];
            if (pGadget->id == TA_GUI_GADGET_TYPE_BUTTON) {
                if (pGadget->button.quickkey != 0 && ta_was_key_pressed(pGame, toupper(pGadget->button.quickkey))) {
                    pEvent->type    = TA_GUI_EVENT_TYPE_BUTTON_PRESSED;
                    pEvent->pGadget = &pGUI->pGadgets[iGadget];
                    return TA_TRUE;
                }
            }
        }
    }



    // Mouse
    // =====
    ta_int32 mousePosXGUI;
    ta_int32 mousePosYGUI;
    ta_gui_map_screen_position(pGUI, pGame->pGraphics->resolutionX, pGame->pGraphics->resolutionY, (ta_int32)pGame->input.mousePosX, (ta_int32)pGame->input.mousePosY, &mousePosXGUI, &mousePosYGUI);

    ta_uint32 iGadgetUnderMouse;
    ta_bool32 isMouseOverGadget = ta_gui_get_gadget_under_point(pGUI, mousePosXGUI, mousePosYGUI, &iGadgetUnderMouse);

    ta_uint32 iHeldGadget;
    ta_bool32 isGadgetHeld = ta_gui_get_held_gadget(pGUI, &iHeldGadget);

    ta_bool32 wasLMBPressed = ta_was_mouse_button_pressed(pGame, TA_MOUSE_BUTTON_LEFT);
    ta_bool32 wasRMBPressed = ta_was_mouse_button_pressed(pGame, TA_MOUSE_BUTTON_RIGHT);
    ta_bool32 wasMBPressed = wasLMBPressed || wasRMBPressed;

    ta_bool32 wasLMBReleased = ta_was_mouse_button_released(pGame, TA_MOUSE_BUTTON_LEFT);
    ta_bool32 wasRMBReleased = ta_was_mouse_button_released(pGame, TA_MOUSE_BUTTON_RIGHT);
    ta_bool32 wasMBReleased = wasLMBReleased || wasRMBReleased;

    ta_uint32 heldMB = 0;
    if (isGadgetHeld) {
        ta_gui_gadget* pHeldGadget = &pGUI->pGadgets[iHeldGadget];
        heldMB = pHeldGadget->heldMB;
        if ((wasLMBReleased && heldMB == TA_MOUSE_BUTTON_LEFT) || (wasRMBReleased && heldMB == TA_MOUSE_BUTTON_RIGHT)) {
            if (wasMBPressed || wasMBReleased) {
                ta_gui_release_hold(pGUI, iHeldGadget);
            }
        }
    }


    if (isMouseOverGadget && pGUI->pGadgets[iGadgetUnderMouse].id != TA_GUI_GADGET_TYPE_ROOT) {
        pGUI->hoveredGadgetIndex = iGadgetUnderMouse;
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadgetUnderMouse];
        if (wasMBPressed) {
            if (!isGadgetHeld) {
                if (pGadget->id == TA_GUI_GADGET_TYPE_BUTTON) {
                    ta_gui_hold_gadget(pGUI, iGadgetUnderMouse, (wasLMBPressed) ? TA_MOUSE_BUTTON_LEFT : TA_MOUSE_BUTTON_RIGHT);
                } else if (pGadget->id == TA_GUI_GADGET_TYPE_LISTBOX) {
                    ta_int32 relativeMousePosX = mousePosXGUI - pGadget->xpos;
                    ta_int32 relativeMousePosY = mousePosYGUI - pGadget->ypos;

                    // The selected item is based on the position of the mouse.
                    ta_int32 iSelectedItem = (ta_int32)(relativeMousePosY / pGame->font.height) + pGadget->listbox.scrollPos;
                    if (iSelectedItem >= (ta_int32)pGadget->listbox.itemCount) {
                        pGadget->listbox.iSelectedItem = (ta_uint32)-1;
                    } else {
                        pGadget->listbox.iSelectedItem = (ta_uint32)iSelectedItem;
                    }

                    // Make sure this is the gadget with keyboard focus.
                    pGUI->focusedGadgetIndex = iGadgetUnderMouse;
                }
            }
        } else if (wasMBReleased) {
            if (isGadgetHeld) {
                ta_gui_gadget* pHeldGadget = &pGUI->pGadgets[iHeldGadget];
                if (iHeldGadget == iGadgetUnderMouse && ((wasLMBReleased && heldMB == TA_MOUSE_BUTTON_LEFT) || (wasRMBReleased && heldMB == TA_MOUSE_BUTTON_RIGHT))) {
                    // The gadget was pressed. May want to post an event here.
                    if (pGadget->id == TA_GUI_GADGET_TYPE_BUTTON) {
                        if (pGadget->button.stages == 0) {
                            pEvent->type = TA_GUI_EVENT_TYPE_BUTTON_PRESSED;
                            pEvent->pGadget = pGadget;
                            return TA_TRUE;
                        } else {
                            assert(pGadget->button.stages > 0);
                            ta_uint32 maxStage = (pGadget->button.stages == 1) ? 2 : pGadget->button.stages;
                            pGadget->button.currentStage = (pGadget->button.currentStage + 1) % maxStage;
                        }
                    }
                }
            }
        }
    } else {
        pGUI->hoveredGadgetIndex = (ta_uint32)-1;
    }

    return TA_FALSE;
}

void ta_step__main_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_MAIN_MENU);
    (void)dt;

    // Input
    // =====
    ta_int32 mousePosXGUI;
    ta_int32 mousePosYGUI;
    ta_gui_map_screen_position(&pGame->mainMenu, pGame->pGraphics->resolutionX, pGame->pGraphics->resolutionY, (ta_int32)pGame->input.mousePosX, (ta_int32)pGame->input.mousePosY, &mousePosXGUI, &mousePosYGUI);

    ta_uint32 iGadgetUnderMouse;
    ta_bool32 isMouseOverGadget = ta_gui_get_gadget_under_point(&pGame->mainMenu, mousePosXGUI, mousePosYGUI, &iGadgetUnderMouse);

    ta_gui_input_event e;
    ta_bool32 hasGUIEvent = ta_handle_gui_input(pGame, &pGame->mainMenu, &e);
    if (hasGUIEvent) {
        if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
            if (strcmp(e.pGadget->name, "SINGLE") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_SP_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "MULTI") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_MP_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "INTRO") == 0) {
                // TODO: Go to intro screen.
                return;
            }
            if (strcmp(e.pGadget->name, "EXIT") == 0) {
                ta_close(pGame);
                return;
            }
        }
    }


    // Rendering
    // =========
    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->mainMenu);
    ta_draw_textf(pGame->pGraphics, &pGame->font, 255, 1, 16, 16,                    "Mouse Position: %d %d", (int)pGame->input.mousePosX, (int)pGame->input.mousePosY);
    ta_draw_textf(pGame->pGraphics, &pGame->font, 255, 1, 16, 16+pGame->font.height, "Mouse Position (GUI): %d %d", mousePosXGUI, mousePosYGUI);

    if (isMouseOverGadget) {
        ta_draw_textf(pGame->pGraphics, &pGame->font, 255, 1, 16, 16+(2*pGame->font.height), "Gadget Under Mouse: %s", pGame->mainMenu.pGadgets[iGadgetUnderMouse].name);
    }

    float scale;
    float offsetX;
    float offsetY;
    ta_gui_get_screen_mapping(&pGame->mainMenu, pGame->pGraphics->resolutionX, pGame->pGraphics->resolutionY, &scale, &offsetX, &offsetY);

    const char* versionStr = "OpenTA v0.1";
    float versionSizeX;
    float versionSizeY;
    ta_font_measure_text(&pGame->fontSmall, scale, versionStr, &versionSizeX, &versionSizeY);
    ta_draw_textf(pGame->pGraphics, &pGame->fontSmall, 255, scale, (pGame->pGraphics->resolutionX - versionSizeX)/2, 300*scale + offsetY, versionStr, pGame->mainMenu.pGadgets[iGadgetUnderMouse].name);
}

void ta_step__sp_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_SP_MENU);
    (void)dt;

    // Input
    // =====
    ta_gui_input_event e;
    ta_bool32 hasGUIEvent = ta_handle_gui_input(pGame, &pGame->spMenu, &e);
    if (hasGUIEvent) {
        if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
            if (strcmp(e.pGadget->name, "NewCamp") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_CAMPAIGN_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "LoadGame") == 0) {
                // TODO: Implement me.
                return;
            }
            if (strcmp(e.pGadget->name, "Skirmish") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_SKIRMISH_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "PrevMenu") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_MAIN_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "Options") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_OPTIONS_MENU);
                return;
            }
        }
    }

    // For some reason the SINGLE.GUI file does not define the "escdefault" which means we'll need to hard code it.
    if (ta_was_key_pressed(pGame, TA_KEY_ESCAPE)) {
        ta_goto_screen(pGame, TA_SCREEN_MAIN_MENU);
        return;
    }


    // Rendering
    // =========
    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->spMenu);
}

void ta_step__mp_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_MP_MENU);
    (void)dt;

    // Input
    // =====
    ta_gui_input_event e;
    ta_bool32 hasGUIEvent = ta_handle_gui_input(pGame, &pGame->mpMenu, &e);
    if (hasGUIEvent) {
        if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
            if (strcmp(e.pGadget->name, "PREVMENU") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_MAIN_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "SETTINGS") == 0) {
                ta_goto_screen(pGame, TA_SCREEN_OPTIONS_MENU);
                return;
            }
            if (strcmp(e.pGadget->name, "SELECT") == 0) {
                // TODO: Implement me.
                return;
            }
        }
    }


    // Rendering
    // =========
    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->mpMenu);
}

void ta_step__options_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_OPTIONS_MENU);
    (void)dt;

    // Input
    // =====
    ta_gui_input_event e;
    ta_bool32 hasGUIEvent = ta_handle_gui_input(pGame, &pGame->optionsMenu, &e);
    if (hasGUIEvent) {
        if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
            if (strcmp(e.pGadget->name, "PREV") == 0) {     // This is the OK button
                ta_goto_screen(pGame, pGame->prevScreen);
                return;
            }
            if (strcmp(e.pGadget->name, "CANCEL") == 0) {
                ta_goto_screen(pGame, pGame->prevScreen);
                return;
            }
        }
    }

    // For some reason this GUI does not define the "escdefault" which means we'll need to hard code it.
    if (ta_was_key_pressed(pGame, TA_KEY_ESCAPE)) {
        ta_goto_screen(pGame, pGame->prevScreen);
        return;
    }


    // Rendering
    // =========
    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->optionsMenu);
}

void ta_step__skirmish_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_SKIRMISH_MENU);
    (void)dt;

    // Input
    // =====
    ta_gui_input_event e;
    ta_bool32 hasGUIEvent;
    if (pGame->pCurrentDialog == NULL) {
        hasGUIEvent = ta_handle_gui_input(pGame, &pGame->skirmishMenu, &e);
        if (hasGUIEvent) {
            if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
                if (strcmp(e.pGadget->name, "PrevMenu") == 0) {
                    ta_goto_screen(pGame, pGame->prevScreen);
                    return;
                }
                if (strcmp(e.pGadget->name, "SelectMap") == 0) {
                    pGame->pCurrentDialog = &pGame->selectMapDialog;
                    return;
                }
                if (strcmp(e.pGadget->name, "Start") == 0) {
                    pGame->pCurrentMap = ta_load_map(pGame, ta_config_get_string(pGame->ppMPMaps[pGame->iSelectedMPMap], "GlobalHeader/missionname"));    // TODO: Free this when the user leaves the game!
                    ta_goto_screen(pGame, TA_SCREEN_IN_GAME);
                    return;
                }
            }
        }
    } else {
        // A dialog is open, so handle the input of that. The skirmish menu only cares about the Select Map dialog.
        assert(pGame->pCurrentDialog == &pGame->selectMapDialog);
        hasGUIEvent = ta_handle_gui_input(pGame, pGame->pCurrentDialog, &e);
        if (hasGUIEvent) {
            if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
                if (strcmp(e.pGadget->name, "PREVMENU") == 0) {
                    pGame->pCurrentDialog = NULL;   // Close the dialog.
                }
                if (strcmp(e.pGadget->name, "LOAD") == 0) {
                    ta_uint32 iMapListGadget;
                    ta_gui_find_gadget_by_name(&pGame->selectMapDialog, "MAPNAMES", &iMapListGadget);

                    pGame->iSelectedMPMap = pGame->selectMapDialog.pGadgets[iMapListGadget].listbox.iSelectedItem;

                    // TODO: Update the map label on the main GUI.

                    pGame->pCurrentDialog = NULL;   // Close the dialog.
                }
            }
        }
    }


    // Rendering
    // =========
    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->skirmishMenu);

    if (pGame->pCurrentDialog != NULL) {
        assert(pGame->pCurrentDialog == &pGame->selectMapDialog);
        ta_draw_dialog_gui(pGame->pGraphics, &pGame->selectMapDialog);
    }
}

void ta_step__campaign_menu(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_CAMPAIGN_MENU);
    (void)dt;

    // Input
    // =====
    ta_gui_input_event e;
    ta_bool32 hasGUIEvent;
    if (pGame->pCurrentDialog == NULL) {
        hasGUIEvent = ta_handle_gui_input(pGame, &pGame->campaignMenu, &e);
        if (hasGUIEvent) {
            if (e.type == TA_GUI_EVENT_TYPE_BUTTON_PRESSED) {
                if (strcmp(e.pGadget->name, "PrevMenu") == 0) {
                    ta_goto_screen(pGame, TA_SCREEN_SP_MENU);
                    return;
                }
                if (strcmp(e.pGadget->name, "Start") == 0) {
                    // TODO: Implement me.
                    return;
                }
            }
        }
    }

    // Rendering
    // =========
    ta_draw_fullscreen_gui(pGame->pGraphics, &pGame->campaignMenu);
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

            case TA_SCREEN_SP_MENU:
            {
                ta_step__sp_menu(pGame, dt);
            } break;

            case TA_SCREEN_MP_MENU:
            {
                ta_step__mp_menu(pGame, dt);
            } break;

            case TA_SCREEN_OPTIONS_MENU:
            {
                ta_step__options_menu(pGame, dt);
            } break;

            case TA_SCREEN_SKIRMISH_MENU:
            {
                ta_step__skirmish_menu(pGame, dt);
            } break;

            case TA_SCREEN_CAMPAIGN_MENU:
            {
                ta_step__campaign_menu(pGame, dt);
            } break;

            case TA_SCREEN_INTRO:
            {
            } break;

            case TA_SCREEN_CREDITS:
            {
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
    pGame->prevScreen = pGame->screen;
    pGame->screen = newScreenType;

    // Make sure any dialog is closed after a fresh switch.
    assert(pGame->pCurrentDialog == NULL);
    pGame->pCurrentDialog = NULL;
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

ta_bool32 ta_was_mouse_button_released(ta_game* pGame, ta_uint32 button)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_mouse_button_released(&pGame->input, button);
}


ta_bool32 ta_is_key_down(ta_game* pGame, ta_uint32 key)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_is_key_down(&pGame->input, key);
}

ta_bool32 ta_was_key_pressed(ta_game* pGame, ta_uint32 key)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_key_pressed(&pGame->input, key);
}

ta_bool32 ta_was_key_released(ta_game* pGame, ta_uint32 key)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_key_released(&pGame->input, key);
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

    if (key < ta_countof(pGame->input.keyState)) {
        if ((stateFlags & TA_KEY_STATE_AUTO_REPEATED) == 0) {
            ta_input_state_on_key_down(&pGame->input, key);
        }
    }
}

void ta_on_key_up(ta_game* pGame, ta_key key, unsigned int stateFlags)
{
    assert(pGame != NULL);

    if (key < ta_countof(pGame->input.keyState)) {
        ta_input_state_on_key_up(&pGame->input, key);
    }
}

void ta_on_printable_key_down(ta_game* pGame, uint32_t utf32, unsigned int stateFlags)
{
    assert(pGame != NULL);
    (void)pGame;
    (void)utf32;
    (void)stateFlags;
}
