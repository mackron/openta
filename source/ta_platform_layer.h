// Public domain. See "unlicense" statement at the end of this file.

#define TA_WINDOW_FULLSCREEN    0x0001


///////////////////////////////////////////////////////////////////////////////
//
// Windows
//
///////////////////////////////////////////////////////////////////////////////

// Structure representing a window.
typedef struct
{
    // A pointer to the game that owns this window.
    ta_game* pGame;

#ifdef _WIN32
    HWND hWnd;
#endif

#ifdef __linux__
    Window x11Window;
#endif

} ta_window;


// Initializes the window system. Only call this once per application.
bool ta_init_window_system();

// Uninitializes the window system.
void ta_uninit_window_system();


// Creates a game window.
#ifdef _WIN32
ta_window* ta_create_window(ta_game* pGame, const char* pTitle, unsigned int width, unsigned int height, unsigned int options);

// Retrieves a handle to the device context of the given window.
HDC ta_get_window_hdc(ta_window* pWindow);
#endif

// Deletes the given window.
void ta_delete_window(ta_window* pWindow);


// Runs the main application loop.
int ta_main_loop(ta_game* pGame);


///////////////////////////////////////////////////////////////////////////////
//
// Timers
//
///////////////////////////////////////////////////////////////////////////////

typedef struct ta_timer ta_timer;

// Creates a high-resolution timer.
ta_timer* ta_create_timer();

// Deletes the given high-resolution timer.
void ta_delete_timer(ta_timer* pTimer);

// Ticks the timer and returns the number of seconds since the previous tick.
double ta_tick_timer(ta_timer* pTimer);