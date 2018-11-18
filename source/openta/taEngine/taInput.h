// Copyright (C) 2018 David Reid. See included LICENSE file.

// A few notes on input:
// - Transient state is reset at the end of each state. It's used for detecting when a mouse button or key was
//   pressed for the first time.
// - Transient state should _not_ be reset from anywhere except taInputStateResetTransientState(). The
//   reason for this is to ensure transient state changes are available for at least a single frame. A scenario
//   where this may become relevant is when a player rapidly presses and then releases a button quickly enough
//   that it all happens between frames and is thus never handled by the stepping routines.


#define TA_MOUSE_BUTTON_LEFT            0
#define TA_MOUSE_BUTTON_RIGHT           1
#define TA_MOUSE_BUTTON_MIDDLE          2
#define TA_MOUSE_BUTTON_4               3
#define TA_MOUSE_BUTTON_5               5

#define TA_KEY_A                        'A'
#define TA_KEY_B                        'B'
#define TA_KEY_C                        'C'
#define TA_KEY_D                        'D'
#define TA_KEY_BACKSPACE                0x08
#define TA_KEY_TAB                      0x09    // '\t'
#define TA_KEY_ENTER                    0x0D
#define TA_KEY_SHIFT                    0x10
#define TA_KEY_ESCAPE                   0x1B
#define TA_KEY_SPACE                    0x20
#define TA_KEY_PAGE_UP                  0x21
#define TA_KEY_PAGE_DOWN                0x22
#define TA_KEY_END                      0x23
#define TA_KEY_HOME                     0x24
#define TA_KEY_ARROW_LEFT               0x25
#define TA_KEY_ARROW_UP                 0x26
#define TA_KEY_ARROW_DOWN               0x27
#define TA_KEY_ARROW_RIGHT              0x28
#define TA_KEY_DELETE                   0x2E



#define TA_MOUSE_BUTTON_STATE_IS_DOWN   (1 << 0)
#define TA_MOUSE_BUTTON_STATE_PRESSED   (1 << 1)    // Transient
#define TA_MOUSE_BUTTON_STATE_RELEASED  (1 << 2)    // Transient

#define TA_KEY_STATE_IS_DOWN            (1 << 0)
#define TA_KEY_STATE_PRESSED            (1 << 1)    // Transient
#define TA_KEY_STATE_RELEASED           (1 << 2)    // Transient

typedef struct
{
    // Mouse
    // =====
    taUInt8 mouseButtonState[8];
    float mousePosX;
    float mousePosY;
    float mouseOffsetX; // The distance the mouse moved since the last frame. This is transient.
    float mouseOffsetY; // ^

    // Keyboard
    // ========
    taUInt8 keyState[256];
} taInputState;

taResult taInputStateInit(taInputState* pState);
taResult taInputStateUninit(taInputState* pState);

// Resets per-frame state.
void taInputStateResetTransientState(taInputState* pState);


// Called by ta_game when the mouse is moved.
void taInputStateOnMouseMove(taInputState* pState, float newMousePosX, float newMousePosY);
void taInputStateOnMouseButtonDown(taInputState* pState, taUInt32 mouseButton);
void taInputStateOnMouseButtonUp(taInputState* pState, taUInt32 mouseButton);
taBool32 taInputStateIsMouseButtonDown(taInputState* pState, taUInt32 mouseButton);
taBool32 taInputStateWasMouseButtonPressed(taInputState* pState, taUInt32 mouseButton);
taBool32 taInputStateWasMouseButtonReleased(taInputState* pState, taUInt32 mouseButton);
taBool32 taInputStateIsAnyMouseButtonDown(taInputState* pState);

// Called when the state of a key changes.
void taInputStateOnKeyDown(taInputState* pState, taUInt32 key);
void taInputStateOnKeyUp(taInputState* pState, taUInt32 key);
taBool32 taInputStateIsKeyDown(taInputState* pState, taUInt32 key);
taBool32 taInputStateWasKeyPressed(taInputState* pState, taUInt32 key);
taBool32 taInputStateWasKeyReleased(taInputState* pState, taUInt32 key);
