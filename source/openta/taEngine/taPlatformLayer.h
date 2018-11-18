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

typedef taUInt32 taKey;


///////////////////////////////////////////////////////////////////////////////
//
// Windows
//
///////////////////////////////////////////////////////////////////////////////

// Structure representing a window.
typedef struct
{
    // The engine context that owns this window.
    taEngineContext* pEngine;

#ifdef _WIN32
    // The main Win32 window handle.
    HWND hWnd;

    // The high-surrogate from a WM_CHAR message. This is used in order to build a surrogate pair from a couple of WM_CHAR messages. When
    // a WM_CHAR message is received when code point is not a high surrogate, this is set to 0.
    unsigned short utf16HighSurrogate;

    // Keeps track of whether or not the cursor is over this window.
    taBool32 isCursorOver;
#endif

#ifdef __linux__
    Window x11Window;
#endif

} taWindow;


// Initializes the window system. Only call this once per application.
taBool32 taInitWindowSystem();

// Uninitializes the window system.
void taUninitWindowSystem();


// Posts a quit message.
void taPostQuitMessage(int exitCode);


// Creates a game window.
#ifdef _WIN32
taWindow* taCreateWindow(taEngineContext* pEngine, const char* pTitle, unsigned int width, unsigned int height, unsigned int options);

// Retrieves a handle to the device context of the given window.
HDC taGetWindowHDC(taWindow* pWindow);
#endif

// Deletes the given window.
void taDeleteWindow(taWindow* pWindow);


// Gives the given window the mouse capture.
void taWindowCaptureMouse(taWindow* pWindow);

// Releases the mouse.
void taWindowReleaseMouse();


// Runs the main application loop.
int taMainLoop(taEngineContext* pEngine);
