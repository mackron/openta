// Copyright (C) 2018 David Reid. See included LICENSE file.

#ifndef TA_GAME_H
#define TA_GAME_H

#include "../taEngine/taEngine.h"

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

typedef struct
{
    taEngineContext engine;

    // The game window.
    taWindow* pWindow;

    // The game timer for stepping the game.
    dr_timer timer;


    // The current map instance. Set to null when there is no map running.
    taMapInstance* pCurrentMap;


    // The main menu.
    taGUI mainMenu;
    taGUI spMenu;
    taGUI mpMenu;
    taGUI optionsMenu;
    taGUI skirmishMenu;
    taGUI campaignMenu;


    taGUI selectMapDialog;
    taGUI* pCurrentDialog; // Set to non-null when a dialog is open, and will be set to a pointer to one of the dialog GUIs.


    // The current screen. This is set to one of TA_SCREEN_*
    taUInt32 screen;
    taUInt32 prevScreen;   // Only used by the options menu for handling the back button.
    

    // The list of multi-player/skirmish maps.
    taConfigObj** ppMPMaps;   // <-- stb_stretchy_buffer

    // The list of single-player maps.
    taConfigObj** ppSPMaps;   // <-- stb_stretchy_buffer

    // The selected multi-player/skirmish map.
    taUInt32 iSelectedMPMap;


    taTexture* pTexture;
} ta_game;

// Creates a game instance.
ta_game* ta_create_game(int argc, char** argv);

// Deletes a game instance.
void ta_delete_game(ta_game* pGame);


// Sets a global property.
taResult ta_set_property(ta_game* pGame, const char* key, const char* value);
taResult ta_set_property_int(ta_game* pGame, const char* key, taInt32 value);
taResult ta_set_property_float(ta_game* pGame, const char* key, float value);
taResult ta_set_property_bool(ta_game* pGame, const char* key, taBool32 value);

// Retrieves a global property. Be careful with the returned pointer because it can become invalid whenever ta_set_property()
// is called. If you need to store the value, make a copy.
const char* ta_get_property(ta_game* pGame, const char* key);
const char* ta_get_propertyf(ta_game* pGame, const char* key, ...);
taInt32 ta_get_property_int(ta_game* pGame, const char* key);
float ta_get_property_float(ta_game* pGame, const char* key);
taBool32 ta_get_property_bool(ta_game* pGame, const char* key);


// Runs the given game.
int ta_game_run(ta_game* pGame);

// Posts a message to quit the game.
void ta_close(ta_game* pGame);


// Changes the screen.
void ta_goto_screen(ta_game* pGame, taUInt32 newScreenType);


// Determines whether or not a mouse button is currently down.
taBool32 ta_is_mouse_button_down(ta_game* pGame, taUInt32 button);

// Determines whether or not a mouse button was just pressed.
taBool32 ta_was_mouse_button_pressed(ta_game* pGame, taUInt32 button);

// Determines whether or not a mouse button was just released.
taBool32 ta_was_mouse_button_released(ta_game* pGame, taUInt32 button);


// Determines whether or not a key is currently down.
taBool32 ta_is_key_down(ta_game* pGame, taUInt32 key);

// Determines whether or not a key was just pressed.
taBool32 ta_was_key_pressed(ta_game* pGame, taUInt32 key);

// Determines whether or not a key was just released.
taBool32 ta_was_key_released(ta_game* pGame, taUInt32 key);



#endif
