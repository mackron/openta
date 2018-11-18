// Copyright (C) 2018 David Reid. See included LICENSE file.

// A few notes on input:
// - Transient state is reset at the end of each state. It's used for detecting when a mouse button or key was
//   pressed for the first time.
// - Transient state should _not_ be reset from anywhere except ta_input_state_reset_transient_state(). The
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

} ta_input_state;

taResult ta_input_state_init(ta_input_state* pState);
taResult ta_input_state_uninit(ta_input_state* pState);

// Resets per-frame state.
void ta_input_state_reset_transient_state(ta_input_state* pState);


// Called by ta_game when the mouse is moved.
void ta_input_state_on_mouse_move(ta_input_state* pState, float newMousePosX, float newMousePosY);
void ta_input_state_on_mouse_button_down(ta_input_state* pState, taUInt32 mouseButton);
void ta_input_state_on_mouse_button_up(ta_input_state* pState, taUInt32 mouseButton);
taBool32 ta_input_state_is_mouse_button_down(ta_input_state* pState, taUInt32 mouseButton);
taBool32 ta_input_state_was_mouse_button_pressed(ta_input_state* pState, taUInt32 mouseButton);
taBool32 ta_input_state_was_mouse_button_released(ta_input_state* pState, taUInt32 mouseButton);
taBool32 ta_input_state_is_any_mouse_button_down(ta_input_state* pState);

// Called when the state of a key changes.
void ta_input_state_on_key_down(ta_input_state* pState, taUInt32 key);
void ta_input_state_on_key_up(ta_input_state* pState, taUInt32 key);
taBool32 ta_input_state_is_key_down(ta_input_state* pState, taUInt32 key);
taBool32 ta_input_state_was_key_pressed(ta_input_state* pState, taUInt32 key);
taBool32 ta_input_state_was_key_released(ta_input_state* pState, taUInt32 key);