// Public domain. See "unlicense" statement at the end of this file.

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
    ta_timer* pTimer;

    // The audio context.
    draudio_context* pAudioContext;


    // The features library. This is initialized once at startup from every TDF file in the "features" directory, and it's sub-directories. The
    // features library is immutable once it's initialized.
    ta_features_library* pFeatures;



    // Whether or not the middle mouse button is down.
    bool isMMBDown;

    // The position of the mouse at the time it was clicked.
    int mouseDownPosX;
    int mouseDownPosY;


    // TEST TEXTURE
    ta_texture* pTexture;
    ta_gaf_entry_frame* pFrame;
    ta_tnt* pTNT;
};

// Creates a game instance.
ta_game* ta_create_game(dr_cmdline cmdline);

// Deletes a game instance.
void ta_delete_game(ta_game* pGame);


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