// Copyright (C) 2018 David Reid. See included LICENSE file.

#include "taGame.h"
#include "../taEngine/taEngine.c"

void ta_game_on_step(taEngineContext* pEngine);
void ta_game_on_load_properties(taEngineContext* pEngine, ta_property_manager* pProperties);

TA_PRIVATE int ta_qsort_cb_map(const void* a, const void* b)
{
    const ta_config_obj** ppOTA_0 = (const ta_config_obj**)a;
    const ta_config_obj** ppOTA_1 = (const ta_config_obj**)b;

    const char* name0 = ta_config_get_string(*ppOTA_0, "GlobalHeader/missionname");
    const char* name1 = ta_config_get_string(*ppOTA_1, "GlobalHeader/missionname");

    return _stricmp(name0, name1);
}


TA_PRIVATE ta_result ta_load_default_totala_settings(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // TODO: Implement me.
    return TA_SUCCESS;
}

TA_PRIVATE ta_result ta_load_totala_settings__config(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // TODO: Implement me.
    return TA_ERROR;
}

#ifdef _WIN32
TA_PRIVATE ta_uint32 ta_registry_read_uint32(HKEY hKey, const char* name)
{
    DWORD value = 0;
    DWORD valueSize = sizeof(value);
    LONG result = RegQueryValueExA(hKey, name, 0, NULL, (LPBYTE)&value, &valueSize);
    if (result != ERROR_SUCCESS) {
        return 0;
    }

    return value;
}

TA_PRIVATE char* ta_registry_read_string(HKEY hKey, const char* name, char* value, size_t valueBufferSize)  // Return value is <value> for convenience (null on error).
{
    LONG result = RegQueryValueExA(hKey, name, 0, NULL, (LPBYTE)value, (LPDWORD)&valueBufferSize);
    if (result != ERROR_SUCCESS) {
        return NULL;
    }

    return value;
}

TA_PRIVATE ta_result ta_load_totala_settings__registry(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    char strBuffer[256];

    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Cavedog Entertainment\\Total Annihilation", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return TA_ERROR;
    }

    ta_property_manager_set_int(pProperties, "totala.ackfx",                    (int)ta_registry_read_uint32(hKey, "ackfx"));
    ta_property_manager_set_int(pProperties, "totala.anti-alias",               (int)ta_registry_read_uint32(hKey, "Anti-Alias"));
    ta_property_manager_set_int(pProperties, "totala.buildfx",                  (int)ta_registry_read_uint32(hKey, "buildfx"));
    ta_property_manager_set_int(pProperties, "totala.cdlists",                  (int)ta_registry_read_uint32(hKey, "CDLISTS"));
    ta_property_manager_set_int(pProperties, "totala.cdmode",                   (int)ta_registry_read_uint32(hKey, "cdmode"));
    ta_property_manager_set_int(pProperties, "totala.cdshell",                  (int)ta_registry_read_uint32(hKey, "cdshell"));
    ta_property_manager_set_int(pProperties, "totala.clock",                    (int)ta_registry_read_uint32(hKey, "clock"));
    ta_property_manager_set_int(pProperties, "totala.damagebars",               (int)ta_registry_read_uint32(hKey, "damagebars"));
    ta_property_manager_set_int(pProperties, "totala.difficulty",               (int)ta_registry_read_uint32(hKey, "Difficulty"));
    ta_property_manager_set_int(pProperties, "totala.display-width",            (int)ta_registry_read_uint32(hKey, "DisplaymodeWidth"));
    ta_property_manager_set_int(pProperties, "totala.display-height",           (int)ta_registry_read_uint32(hKey, "DisplaymodeHeight"));
    ta_property_manager_set_int(pProperties, "totala.dithered-fog",             (int)ta_registry_read_uint32(hKey, "DitheredFog"));
    ta_property_manager_set_int(pProperties, "totala.feature-shadows",          (int)ta_registry_read_uint32(hKey, "FeatureShadows"));
    ta_property_manager_set_int(pProperties, "totala.fixed-locations",          (int)ta_registry_read_uint32(hKey, "FixedLocations"));
    ta_property_manager_set_int(pProperties, "totala.fxvol",                    (int)ta_registry_read_uint32(hKey, "fxvol"));
    ta_property_manager_set(    pProperties, "totala.game-name",                     ta_registry_read_string(hKey, "Game Name", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set_int(pProperties, "totala.game-speed",               (int)ta_registry_read_uint32(hKey, "gamespeed"));
    ta_property_manager_set_int(pProperties, "totala.gamma",                    (int)ta_registry_read_uint32(hKey, "Gamma"));
    ta_property_manager_set_int(pProperties, "totala.interface-type",           (int)ta_registry_read_uint32(hKey, "Interface Type"));
    ta_property_manager_set_int(pProperties, "totala.mixing-buffers",           (int)ta_registry_read_uint32(hKey, "MixingBuffers"));
    ta_property_manager_set_int(pProperties, "totala.mouse-speed",              (int)ta_registry_read_uint32(hKey, "mousespeed"));
    ta_property_manager_set_int(pProperties, "totala.multi-commander-death",    (int)ta_registry_read_uint32(hKey, "MultiCommanderDeath"));
    ta_property_manager_set_int(pProperties, "totala.multi-line-of-sight",      (int)ta_registry_read_uint32(hKey, "MultiLineOfSight"));
    ta_property_manager_set_int(pProperties, "totala.multi-los-type",           (int)ta_registry_read_uint32(hKey, "MultiLOSType"));
    ta_property_manager_set_int(pProperties, "totala.multi-mapping",            (int)ta_registry_read_uint32(hKey, "MultiMapping"));
    ta_property_manager_set_int(pProperties, "totala.music-mode",               (int)ta_registry_read_uint32(hKey, "musicmode"));
    ta_property_manager_set_int(pProperties, "totala.musicvol",                 (int)ta_registry_read_uint32(hKey, "musicvol"));
    ta_property_manager_set(    pProperties, "totala.nickname",                      ta_registry_read_string(hKey, "Nickname", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set(    pProperties, "totala.password",                      ta_registry_read_string(hKey, "Password", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set_int(pProperties, "totala.play-movie",               (int)ta_registry_read_uint32(hKey, "PlayMovie"));
    ta_property_manager_set_int(pProperties, "totala.restore-volume",           (int)ta_registry_read_uint32(hKey, "RestoreVolume"));
    ta_property_manager_set_int(pProperties, "totala.screen-chat",              (int)ta_registry_read_uint32(hKey, "screenchat"));
    ta_property_manager_set_int(pProperties, "totala.scroll-speed",             (int)ta_registry_read_uint32(hKey, "scrollspeed"));
    ta_property_manager_set_int(pProperties, "totala.shading",                  (int)ta_registry_read_uint32(hKey, "Shading"));
    ta_property_manager_set_int(pProperties, "totala.shadows",                  (int)ta_registry_read_uint32(hKey, "Shadows"));
    ta_property_manager_set_int(pProperties, "totala.side",                     (int)ta_registry_read_uint32(hKey, "side"));
    ta_property_manager_set_int(pProperties, "totala.single-commander-death",   (int)ta_registry_read_uint32(hKey, "SingleCommanderDeath"));
    ta_property_manager_set_int(pProperties, "totala.single-line-of-sight",     (int)ta_registry_read_uint32(hKey, "SingleLineOfSight"));
    ta_property_manager_set_int(pProperties, "totala.single-los-type",          (int)ta_registry_read_uint32(hKey, "SingleLOSType"));
    ta_property_manager_set_int(pProperties, "totala.single-mapping",           (int)ta_registry_read_uint32(hKey, "SingleMapping"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-commander-death", (int)ta_registry_read_uint32(hKey, "SkirmishCommanderDeath"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-difficulty",      (int)ta_registry_read_uint32(hKey, "SkirmishDifficulty"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-line-of-sight",   (int)ta_registry_read_uint32(hKey, "SkirmishLineOfSight"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-location",        (int)ta_registry_read_uint32(hKey, "SkirmishLocation"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-los-type",        (int)ta_registry_read_uint32(hKey, "SkirmishLOSType"));
    ta_property_manager_set(    pProperties, "totala.skirmish-map",                  ta_registry_read_string(hKey, "SkirmishMap", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set_int(pProperties, "totala.skirmish-mapping",         (int)ta_registry_read_uint32(hKey, "SkirmishMapping"));
    ta_property_manager_set_int(pProperties, "totala.sound-mode",               (int)ta_registry_read_uint32(hKey, "Sound Mode"));
    ta_property_manager_set_int(pProperties, "totala.speechfx",                 (int)ta_registry_read_uint32(hKey, "speechfx"));
    ta_property_manager_set_int(pProperties, "totala.switch-alt",               (int)ta_registry_read_uint32(hKey, "SwitchAlt"));
    ta_property_manager_set_int(pProperties, "totala.text-lines",               (int)ta_registry_read_uint32(hKey, "textlines"));
    ta_property_manager_set_int(pProperties, "totala.text-scroll",              (int)ta_registry_read_uint32(hKey, "textscroll"));
    ta_property_manager_set_int(pProperties, "totala.unit-chat",                (int)ta_registry_read_uint32(hKey, "unitchat"));
    ta_property_manager_set_int(pProperties, "totala.unit-chat-text",           (int)ta_registry_read_uint32(hKey, "unitchattext"));
    ta_property_manager_set_int(pProperties, "totala.vehicle-shadows",          (int)ta_registry_read_uint32(hKey, "VehicleShadows"));

    RegCloseKey(hKey);


    result = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Cavedog Entertainment\\Total Annihilation\\Skirmish", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return TA_ERROR;
    }

    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-ally-group", (int)ta_registry_read_uint32(hKey, "Player0AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-color",      (int)ta_registry_read_uint32(hKey, "Player0Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-controller", (int)ta_registry_read_uint32(hKey, "Player0Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-energy",     (int)ta_registry_read_uint32(hKey, "Player0Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-metal",      (int)ta_registry_read_uint32(hKey, "Player0Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-side",       (int)ta_registry_read_uint32(hKey, "Player0Side"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-ally-group", (int)ta_registry_read_uint32(hKey, "Player1AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-color",      (int)ta_registry_read_uint32(hKey, "Player1Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-controller", (int)ta_registry_read_uint32(hKey, "Player1Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-energy",     (int)ta_registry_read_uint32(hKey, "Player1Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-metal",      (int)ta_registry_read_uint32(hKey, "Player1Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-side",       (int)ta_registry_read_uint32(hKey, "Player1Side"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-ally-group", (int)ta_registry_read_uint32(hKey, "Player2AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-color",      (int)ta_registry_read_uint32(hKey, "Player2Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-controller", (int)ta_registry_read_uint32(hKey, "Player2Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-energy",     (int)ta_registry_read_uint32(hKey, "Player2Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-metal",      (int)ta_registry_read_uint32(hKey, "Player2Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-side",       (int)ta_registry_read_uint32(hKey, "Player2Side"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-ally-group", (int)ta_registry_read_uint32(hKey, "Player3AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-color",      (int)ta_registry_read_uint32(hKey, "Player3Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-controller", (int)ta_registry_read_uint32(hKey, "Player3Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-energy",     (int)ta_registry_read_uint32(hKey, "Player3Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-metal",      (int)ta_registry_read_uint32(hKey, "Player3Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-side",       (int)ta_registry_read_uint32(hKey, "Player3Side"));

    RegCloseKey(hKey);

    return TA_SUCCESS;
}
#endif

TA_PRIVATE ta_result ta_load_totala_settings(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // Load defaults first, and then overwrite them with the actual settings from the config file or registry.
    ta_result result = ta_load_default_totala_settings(pProperties);
    if (result != TA_SUCCESS) {
        return result;
    }

    // Try loading the original Total Annihilation settings from a config file first. If that fails and we're running on Windows,
    // try the registry.
    result = ta_load_totala_settings__config(pProperties);
    if (result != TA_SUCCESS) {
#ifdef _WIN32
        return ta_load_totala_settings__registry(pProperties);
#else
        return result;
#endif
    }

    return TA_SUCCESS;
}

TA_PRIVATE ta_result ta_load_settings(ta_property_manager* pProperties)
{
    if (pProperties == NULL) return TA_INVALID_ARGS;

    // Original Total Annihilation settings from the registry.
    ta_result result = ta_load_totala_settings(pProperties);
    if (result != TA_SUCCESS) {
        return result;
    }

    // Here is where OpenTA-specific properties would be loaded from openta.cfg.

    return TA_SUCCESS;
}


TA_PRIVATE ta_result ta_save_totala_settings(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // Just save these to totala.cfg for now, but may want to consider writing to the registry
    // for compatibility with totala.exe later on.

    // TODO: Implement me.
    return TA_SUCCESS;
}

TA_PRIVATE ta_result ta_save_settings(ta_property_manager* pProperties)
{
    if (pProperties != NULL) return TA_INVALID_ARGS;

    // Save the original Total Annihilation settings first.
    ta_save_totala_settings(pProperties);

    // Here is where OpenTA-specific properties should be saved to openta.cfg.

    return TA_SUCCESS;
}

void ta_game_on_load_properties(taEngineContext* pEngine, ta_property_manager* pProperties)
{
    (void)pEngine;
    ta_load_settings(pProperties);
}



ta_game* ta_create_game(int argc, char** argv)
{
    ta_game* pGame = calloc(1, sizeof(*pGame));
    if (pGame == NULL) {
        return NULL;
    }

    ta_result result = taEngineContextInit(argc, argv, ta_game_on_load_properties, ta_game_on_step, pGame, &pGame->engine);
    if (result != TA_SUCCESS) {
        free(pGame);
        return NULL;
    }



    // Create and show the window as soon as we can to make loading feel faster.
    pGame->pWindow = ta_graphics_create_window(pGame->engine.pGraphics, "Total Annihilation", 1280, 720, TA_WINDOW_FULLSCREEN | TA_WINDOW_CENTERED);
    if (pGame->pWindow == NULL) {
        goto on_error;
    }


    // Menus.
    ta_set_property(pGame, "MAINMENU.GUI.BACKGROUND", "bitmaps/FrontendX.pcx");
    if (ta_gui_load(&pGame->engine, "guis/MAINMENU.GUI", &pGame->mainMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SINGLE.GUI.BACKGROUND", "bitmaps/SINGLEBG.PCX");
    if (ta_gui_load(&pGame->engine, "guis/SINGLE.GUI", &pGame->spMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SELPROV.GUI.BACKGROUND", "bitmaps/selconnect2.pcx");
    if (ta_gui_load(&pGame->engine, "guis/SELPROV.GUI", &pGame->mpMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "STARTOPT.GUI.BACKGROUND", "bitmaps/options4x.pcx");
    if (ta_gui_load(&pGame->engine, "guis/STARTOPT.GUI", &pGame->optionsMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SKIRMISH.GUI.BACKGROUND", "bitmaps/Skirmsetup4x.pcx");
    if (ta_gui_load(&pGame->engine, "guis/SKIRMISH.GUI", &pGame->skirmishMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "NEWGAME.GUI.BACKGROUND", "bitmaps/playanygame4.pcx");
    if (ta_gui_load(&pGame->engine, "guis/NEWGAME.GUI", &pGame->campaignMenu) != TA_SUCCESS) {
        goto on_error;
    }

    ta_set_property(pGame, "SELMAP.GUI.BACKGROUND", "bitmaps/DSelectmap2.pcx");
    if (ta_gui_load(&pGame->engine, "guis/SELMAP.GUI", &pGame->selectMapDialog) != TA_SUCCESS) {
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
    ta_fs_iterator* pIter = ta_fs_begin(pGame->engine.pFS, "maps", TA_FALSE);
    do
    {
        // From what I can tell, it looks like skirmish/mp maps are determined by the "type" property of the first
        // schema in the OTA file.
        if (drpath_extension_equal(pIter->fileInfo.relativePath, "ota")) {
            ta_config_obj* pOTA = ta_parse_config_from_file(pGame->engine.pFS, pIter->fileInfo.relativePath);
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
        taEngineContextUninit(&pGame->engine);
    }

    return NULL;
}

void ta_delete_game(ta_game* pGame)
{
    if (pGame == NULL) {
        return;
    }

    ta_delete_window(pGame->pWindow);
    taEngineContextUninit(&pGame->engine);
    free(pGame);
}


ta_result ta_set_property(ta_game* pGame, const char* key, const char* value)
{
    return ta_property_manager_set(&pGame->engine.properties, key, value);
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
    return ta_property_manager_get(&pGame->engine.properties, key);
}

const char* ta_get_propertyf(ta_game* pGame, const char* key, ...)
{
    va_list args;
    va_start(args, key);

    const char* value = ta_property_manager_getv(&pGame->engine.properties, key, args);

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

    return ta_main_loop(&pGame->engine);
}

void ta_close(ta_game* pGame)
{
    ta_post_quit_message(0);
}


void ta_step__in_game(ta_game* pGame, double dt)
{
    assert(pGame != NULL);
    assert(pGame->screen == TA_SCREEN_IN_GAME);
    (void)dt;

    if (ta_is_mouse_button_down(pGame, TA_MOUSE_BUTTON_MIDDLE)) {
        ta_translate_camera(pGame->engine.pGraphics, -(int)pGame->engine.input.mouseOffsetX, -(int)pGame->engine.input.mouseOffsetY);
    }

    if (pGame->pCurrentMap) {
        ta_draw_map(pGame->engine.pGraphics, pGame->pCurrentMap);
    }
}


typedef struct
{
    ta_uint32 type;
    ta_gui_gadget* pGadget;  // The gadget this event relates to.
} ta_gui_input_event;

ta_bool32 ta_handle_gui_input(ta_game* pGame, ta_gui* pGUI, ta_gui_input_event* pEvent)
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
    ta_gui_map_screen_position(pGUI, pGame->engine.pGraphics->resolutionX, pGame->engine.pGraphics->resolutionY, (ta_int32)pGame->engine.input.mousePosX, (ta_int32)pGame->engine.input.mousePosY, &mousePosXGUI, &mousePosYGUI);

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
                    ta_int32 iSelectedItem = (ta_int32)(relativeMousePosY / pGame->engine.font.height) + pGadget->listbox.scrollPos;
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
    ta_gui_map_screen_position(&pGame->mainMenu, pGame->engine.pGraphics->resolutionX, pGame->engine.pGraphics->resolutionY, (ta_int32)pGame->engine.input.mousePosX, (ta_int32)pGame->engine.input.mousePosY, &mousePosXGUI, &mousePosYGUI);

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
    ta_draw_fullscreen_gui(pGame->engine.pGraphics, &pGame->mainMenu);
    ta_draw_textf(pGame->engine.pGraphics, &pGame->engine.font, 255, 1, 16, 16,                           "Mouse Position: %d %d", (int)pGame->engine.input.mousePosX, (int)pGame->engine.input.mousePosY);
    ta_draw_textf(pGame->engine.pGraphics, &pGame->engine.font, 255, 1, 16, 16+pGame->engine.font.height, "Mouse Position (GUI): %d %d", mousePosXGUI, mousePosYGUI);

    if (isMouseOverGadget) {
        ta_draw_textf(pGame->engine.pGraphics, &pGame->engine.font, 255, 1, 16, 16+(2*pGame->engine.font.height), "Gadget Under Mouse: %s", pGame->mainMenu.pGadgets[iGadgetUnderMouse].name);
    }

    float scale;
    float offsetX;
    float offsetY;
    ta_gui_get_screen_mapping(&pGame->mainMenu, pGame->engine.pGraphics->resolutionX, pGame->engine.pGraphics->resolutionY, &scale, &offsetX, &offsetY);

    const char* versionStr = "OpenTA v0.1";
    float versionSizeX;
    float versionSizeY;
    ta_font_measure_text(&pGame->engine.fontSmall, scale, versionStr, &versionSizeX, &versionSizeY);
    ta_draw_textf(pGame->engine.pGraphics, &pGame->engine.fontSmall, 255, scale, (pGame->engine.pGraphics->resolutionX - versionSizeX)/2, 300*scale + offsetY, "%s", versionStr);
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
    ta_draw_fullscreen_gui(pGame->engine.pGraphics, &pGame->spMenu);
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
    ta_draw_fullscreen_gui(pGame->engine.pGraphics, &pGame->mpMenu);
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
    ta_draw_fullscreen_gui(pGame->engine.pGraphics, &pGame->optionsMenu);
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
                    pGame->pCurrentMap = ta_load_map(&pGame->engine, ta_config_get_string(pGame->ppMPMaps[pGame->iSelectedMPMap], "GlobalHeader/missionname"));    // TODO: Free this when the user leaves the game!
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
    ta_draw_fullscreen_gui(pGame->engine.pGraphics, &pGame->skirmishMenu);

    if (pGame->pCurrentDialog != NULL) {
        assert(pGame->pCurrentDialog == &pGame->selectMapDialog);
        ta_draw_dialog_gui(pGame->engine.pGraphics, &pGame->selectMapDialog);
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
    ta_draw_fullscreen_gui(pGame->engine.pGraphics, &pGame->campaignMenu);
}

void ta_step__main(ta_game* pGame)
{
    assert(pGame != NULL);

    const double dt = dr_timer_tick(&pGame->timer);
    ta_graphics_set_current_window(pGame->engine.pGraphics, pGame->pWindow);
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
        ta_input_state_reset_transient_state(&pGame->engine.input);
    }
    ta_graphics_present(pGame->engine.pGraphics, pGame->pWindow);
}

void ta_game_on_step(taEngineContext* pEngine)
{
    ta_step__main((ta_game*)pEngine->pUserData);
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
    return ta_input_state_is_mouse_button_down(&pGame->engine.input, button);
}

ta_bool32 ta_was_mouse_button_pressed(ta_game* pGame, ta_uint32 button)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_mouse_button_pressed(&pGame->engine.input, button);
}

ta_bool32 ta_was_mouse_button_released(ta_game* pGame, ta_uint32 button)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_mouse_button_released(&pGame->engine.input, button);
}


ta_bool32 ta_is_key_down(ta_game* pGame, ta_uint32 key)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_is_key_down(&pGame->engine.input, key);
}

ta_bool32 ta_was_key_pressed(ta_game* pGame, ta_uint32 key)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_key_pressed(&pGame->engine.input, key);
}

ta_bool32 ta_was_key_released(ta_game* pGame, ta_uint32 key)
{
    if (pGame == NULL) return TA_FALSE;
    return ta_input_state_was_key_released(&pGame->engine.input, key);
}




