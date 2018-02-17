// Copyright (C) 2018 David Reid. See included LICENSE file.

#define TA_SCREEN_NONE          0
#define TA_SCREEN_IN_GAME       1
#define TA_SCREEN_MAIN_MENU     2
#define TA_SCREEN_SP_MENU       3
#define TA_SCREEN_MP_MENU       4
#define TA_SCREEN_INTRO         5
#define TA_SCREEN_CREDITS       6
#define TA_SCREEN_OPTIONS_MENU  7
#define TA_SCREEN_SKIRMISH_MENU 8
#define TA_SCREEN_CAMPAIGN_MENU 9

struct ta_game
{
    taEngineContext engine;

    // The game window.
    ta_window* pWindow;

    // The game timer for stepping the game.
    dr_timer timer;

    // The audio context.
    ta_audio_context* pAudio;

    // Global properties.
    ta_property_manager properties;


    // The features library. This is initialized once at startup from every TDF file in the "features" directory, and it's sub-directories. The
    // features library is immutable once it's initialized.
    ta_features_library* pFeatures;

    // The current map instance. Set to null when there is no map running.
    ta_map_instance* pCurrentMap;


    // The number of opened texture GAFs.
    uint32_t textureGAFCount;

    // The list of GAF files containing textures. This is initialized when the game context is created.
    ta_gaf** ppTextureGAFs;



    // The Common GUI.
    ta_common_gui commonGUI;

    // The main menu.
    ta_gui mainMenu;
    ta_gui spMenu;
    ta_gui mpMenu;
    ta_gui optionsMenu;
    ta_gui skirmishMenu;
    ta_gui campaignMenu;


    ta_gui selectMapDialog;
    ta_gui* pCurrentDialog; // Set to non-null when a dialog is open, and will be set to a pointer to one of the dialog GUIs.


    // The current screen. This is set to one of TA_SCREEN_*
    ta_uint32 screen;
    ta_uint32 prevScreen;   // Only used by the options menu for handling the back button.
    

    // The list of multi-player/skirmish maps.
    ta_config_obj** ppMPMaps;   // <-- stb_stretchy_buffer

    // The list of single-player maps.
    ta_config_obj** ppSPMaps;   // <-- stb_stretchy_buffer

    // The selected multi-player/skirmish map.
    ta_uint32 iSelectedMPMap;


    ta_texture* pTexture;
};

// Creates a game instance.
ta_game* ta_create_game(int argc, char** argv);

// Deletes a game instance.
void ta_delete_game(ta_game* pGame);


// Sets a global property.
ta_result ta_set_property(ta_game* pGame, const char* key, const char* value);
ta_result ta_set_property_int(ta_game* pGame, const char* key, ta_int32 value);
ta_result ta_set_property_float(ta_game* pGame, const char* key, float value);
ta_result ta_set_property_bool(ta_game* pGame, const char* key, ta_bool32 value);

// Retrieves a global property. Be careful with the returned pointer because it can become invalid whenever ta_set_property()
// is called. If you need to store the value, make a copy.
const char* ta_get_property(ta_game* pGame, const char* key);
const char* ta_get_propertyf(ta_game* pGame, const char* key, ...);
ta_int32 ta_get_property_int(ta_game* pGame, const char* key);
float ta_get_property_float(ta_game* pGame, const char* key);
ta_bool32 ta_get_property_bool(ta_game* pGame, const char* key);


// Runs the given game.
int ta_game_run(ta_game* pGame);

// Posts a message to quit the game.
void ta_close(ta_game* pGame);


// Changes the screen.
void ta_goto_screen(ta_game* pGame, ta_uint32 newScreenType);


// Determines whether or not a mouse button is currently down.
ta_bool32 ta_is_mouse_button_down(ta_game* pGame, ta_uint32 button);

// Determines whether or not a mouse button was just pressed.
ta_bool32 ta_was_mouse_button_pressed(ta_game* pGame, ta_uint32 button);

// Determines whether or not a mouse button was just released.
ta_bool32 ta_was_mouse_button_released(ta_game* pGame, ta_uint32 button);


// Determines whether or not a key is currently down.
ta_bool32 ta_is_key_down(ta_game* pGame, ta_uint32 key);

// Determines whether or not a key was just pressed.
ta_bool32 ta_was_key_pressed(ta_game* pGame, ta_uint32 key);

// Determines whether or not a key was just released.
ta_bool32 ta_was_key_released(ta_game* pGame, ta_uint32 key);



// Creates a texture from a file.
ta_texture* ta_load_image(ta_game* pGame, const char* filePath);


