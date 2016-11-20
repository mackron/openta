// Copyright (C) 2016 David Reid. See included LICENSE file.

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


    // The features library. This is initialized once at startup from every TDF file in the "features" directory, and it's sub-directories. The
    // features library is immutable once it's initialized.
    ta_features_library* pFeatures;

    // The current map instance. Set to null when there is no map running.
    ta_map_instance* pCurrentMap;


    // The number of opened texture GAFs.
    uint32_t textureGAFCount;

    // The list of GAF files containing textures. This is initialized when the game context is created.
    ta_gaf** ppTextureGAFs;


    // The common GUI GAF.
    ta_gaf* pCommonGUIGAF;

    // The main font to use for basically all GUI text.
    ta_font font;

    // The main menu.
    ta_gui mainMenu;
    

    // Whether or not the middle mouse button is down.
    ta_bool32 isMMBDown;

    // The position of the mouse at the time it was clicked.
    int mouseDownPosX;
    int mouseDownPosY;


    ta_texture* pTexture;
};

// Creates a game instance.
ta_game* ta_create_game(dr_cmdline cmdline);

// Deletes a game instance.
void ta_delete_game(ta_game* pGame);


// Sets a global property.
ta_result ta_set_property(ta_game* pGame, const char* key, const char* value);

// Retrieves a global property. Be careful with the returned pointer because it can become invalid whenever ta_set_property()
// is called. If you need to store the value, make a copy.
const char* ta_get_property(ta_game* pGame, const char* key);
const char* ta_get_propertyf(ta_game* pGame, const char* key, ...);


// Runs the given game.
int ta_game_run(ta_game* pGame);

// Steps and renders a single frame.
void ta_do_frame(ta_game* pGame);


// Steps the game.
void ta_game_step(ta_game* pGame);

// Renders the game.
void ta_game_render(ta_game* pGame);


// Captures the mouse so that all mouse events get directed to the game window.
void ta_capture_mouse(ta_game* pGame);

// Releases the mouse. The opposite of ta_capture_mouse().
void ta_release_mouse(ta_game* pGame);


// Creates a texture from a file.
ta_texture* ta_load_image(ta_game* pGame, const char* filePath);


//// GUI ////

// Retrieves the texture to use for the background of a button with the given dimensions and state.
//
// Button graphics are handled a bit strangely in Total Annihilation because they are selected based on the dimensions
// of the button.
//
// The <state> parameter should be set to one of the TA_GUI_BUTTON_STATE_* values.
ta_texture* ta_get_gui_button_texture(ta_game* pGame, ta_uint32 width, ta_uint32 height, ta_uint32 state, ta_subtexture_metrics* pMetrics);


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