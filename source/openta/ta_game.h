// Copyright (C) 2016 David Reid. See included LICENSE file.

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
    // The command line that was used to start up the game.
    dr_cmdline cmdline;

    // The file system context.
    ta_fs* pFS;

    // The graphics context.
    ta_graphics_context* pGraphics;

    // The standard palette. PALETTE.PAL
    uint32_t palette[256];

    // The GUI palette. GUIPAL.PAL
    uint32_t guipal[256];

    // The game window.
    ta_window* pWindow;

    // The game timer for stepping the game.
    dr_timer timer;

    // The audio context.
    dra_context* pAudioContext;

    // Global properties.
    ta_property_manager properties;

    // Whether or not the game is closing.
    ta_bool32 isClosing;


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

    // The main font to use for basically all GUI text.
    ta_font font;
    ta_font fontSmall;

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
    

    // Input state.
    ta_input_state input;




    ta_texture* pTexture;
};

// Creates a game instance.
ta_game* ta_create_game(dr_cmdline cmdline);

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

// Exists from the main loop.
void ta_close(ta_game* pGame);

// Steps and renders a single frame.
void ta_step(ta_game* pGame);


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



// Captures the mouse so that all mouse events get directed to the game window.
void ta_capture_mouse(ta_game* pGame);

// Releases the mouse. The opposite of ta_capture_mouse().
void ta_release_mouse(ta_game* pGame);


// Creates a texture from a file.
ta_texture* ta_load_image(ta_game* pGame, const char* filePath);


//// Events from Window ////

// Called from the window system when the game window is resized.
void ta_on_window_size(ta_game* pGame, unsigned int newWidth, unsigned int newHeight);

// Called when a mouse button is pressed.
void ta_on_mouse_button_down(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags);

// Called when a mouse button is released.
void ta_on_mouse_button_up(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags);

// Called when a mouse button is double clicked.
void ta_on_mouse_button_dblclick(ta_game* pGame, int button, int posX, int posY, unsigned int stateFlags);

// Called when the mouse wheel is turned.
void ta_on_mouse_wheel(ta_game* pGame, int delta, int posX, int posY, unsigned int stateFlags);

// Called when the mouse moves.
void ta_on_mouse_move(ta_game* pGame, int posX, int posY, unsigned int stateFlags);

// Called when the mouse enters the window.
void ta_on_mouse_enter(ta_game* pGame);

// Called when the mouse leaves the window.
void ta_on_mouse_leave(ta_game* pGame);

// Called when a key is pressed.
void ta_on_key_down(ta_game* pGame, ta_key key, unsigned int stateFlags);

// Called when a key is released.
void ta_on_key_up(ta_game* pGame, ta_key key, unsigned int stateFlags);

// Called when a printable key is pressed or auto-repeated. Use this for text boxes.
void ta_on_printable_key_down(ta_game* pGame, uint32_t utf32, unsigned int stateFlags);