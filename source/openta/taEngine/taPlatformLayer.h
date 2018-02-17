// Copyright (C) 2018 David Reid. See included LICENSE file.

#define TA_WINDOW_CENTERED          0x0001
#define TA_WINDOW_FULLSCREEN        0x0002

// Key state flags.
#define TA_MOUSE_BUTTON_LEFT_DOWN   (1 << 0)
#define TA_MOUSE_BUTTON_RIGHT_DOWN  (1 << 1)
#define TA_MOUSE_BUTTON_MIDDLE_DOWN (1 << 2)
#define TA_MOUSE_BUTTON_4_DOWN      (1 << 3)
#define TA_MOUSE_BUTTON_5_DOWN      (1 << 4)
#define TA_KEY_STATE_SHIFT_DOWN     (1 << 5)        // Whether or not a shift key is down at the time the input event is handled.
#define TA_KEY_STATE_CTRL_DOWN      (1 << 6)        // Whether or not a ctrl key is down at the time the input event is handled.
#define TA_KEY_STATE_ALT_DOWN       (1 << 7)        // Whether or not an alt key is down at the time the input event is handled.
#define TA_KEY_STATE_AUTO_REPEATED  (1 << 31)       // Whether or not the key press is generated due to auto-repeating. Only used with key down events.

typedef uint32_t ta_key;


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
    // The main Win32 window handle.
    HWND hWnd;

    // The high-surrogate from a WM_CHAR message. This is used in order to build a surrogate pair from a couple of WM_CHAR messages. When
    // a WM_CHAR message is received when code point is not a high surrogate, this is set to 0.
    unsigned short utf16HighSurrogate;

    // Keeps track of whether or not the cursor is over this window.
    ta_bool32 isCursorOver;
#endif

#ifdef __linux__
    Window x11Window;
#endif

} ta_window;


// Initializes the window system. Only call this once per application.
ta_bool32 ta_init_window_system();

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


// Gives the given window the mouse capture.
void ta_window_capture_mouse(ta_window* pWindow);

// Releases the mouse.
void ta_window_release_mouse();


// Runs the main application loop.
int ta_main_loop(ta_game* pGame);