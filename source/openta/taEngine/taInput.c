// Copyright (C) 2018 David Reid. See included LICENSE file.

taResult ta_input_state_init(ta_input_state* pState)
{
    if (pState == NULL) return TA_INVALID_ARGS;
    taZeroObject(pState);

    return TA_SUCCESS;
}

taResult ta_input_state_uninit(ta_input_state* pState)
{
    if (pState == NULL) return TA_INVALID_ARGS;
    return TA_SUCCESS;
}


void ta_input_state_reset_transient_state(ta_input_state* pState)
{
    if (pState == NULL) return;

    // TOOD: Unroll and SIMD-ify these loops if it becomes a performance problem (unlikely).

    // Mouse
    // =====
    for (taUInt32 i = 0; i < taCountOf(pState->mouseButtonState); ++i) {
        pState->mouseButtonState[i] &= ~(TA_MOUSE_BUTTON_STATE_PRESSED | TA_MOUSE_BUTTON_STATE_RELEASED);
    }

    pState->mouseOffsetX = 0;
    pState->mouseOffsetY = 0;


    // Keyboard
    // ========
    for (taUInt32 i = 0; i < taCountOf(pState->keyState); ++i) {
        pState->keyState[i] &= ~(TA_KEY_STATE_PRESSED | TA_KEY_STATE_RELEASED);
    }
}


void ta_input_state_on_mouse_move(ta_input_state* pState, float newMousePosX, float newMousePosY)
{
    if (pState == NULL) return;

    pState->mouseOffsetX = newMousePosX - pState->mousePosX;
    pState->mouseOffsetY = newMousePosY - pState->mousePosY;
    pState->mousePosX    = newMousePosX;
    pState->mousePosY    = newMousePosY;
}

void ta_input_state_on_mouse_button_down(ta_input_state* pState, taUInt32 mouseButton)
{
    if (pState == NULL) return;
    pState->mouseButtonState[mouseButton] |= (TA_MOUSE_BUTTON_STATE_IS_DOWN | TA_MOUSE_BUTTON_STATE_PRESSED);
}

void ta_input_state_on_mouse_button_up(ta_input_state* pState, taUInt32 mouseButton)
{
    if (pState == NULL) return;
    pState->mouseButtonState[mouseButton] &= ~(TA_MOUSE_BUTTON_STATE_IS_DOWN);
    pState->mouseButtonState[mouseButton] |= TA_MOUSE_BUTTON_STATE_RELEASED;
}

taBool32 ta_input_state_is_mouse_button_down(ta_input_state* pState, taUInt32 mouseButton)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_IS_DOWN) != 0;
}

taBool32 ta_input_state_was_mouse_button_pressed(ta_input_state* pState, taUInt32 mouseButton)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_PRESSED) != 0;
}

taBool32 ta_input_state_was_mouse_button_released(ta_input_state* pState, taUInt32 mouseButton)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_RELEASED) != 0;
}

taBool32 ta_input_state_is_any_mouse_button_down(ta_input_state* pState)
{
    if (pState == NULL) return TA_FALSE;

    for (taUInt32 i = 0; i < taCountOf(pState->mouseButtonState); ++i) {
        if (ta_input_state_is_mouse_button_down(pState, i)) {
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}



void ta_input_state_on_key_down(ta_input_state* pState, taUInt32 key)
{
    if (pState == NULL) return;
    pState->keyState[key] |= (TA_KEY_STATE_IS_DOWN | TA_KEY_STATE_PRESSED);
}

void ta_input_state_on_key_up(ta_input_state* pState, taUInt32 key)
{
    pState->keyState[key] &= ~(TA_KEY_STATE_IS_DOWN);
    pState->keyState[key] |= TA_KEY_STATE_RELEASED;
}

taBool32 ta_input_state_is_key_down(ta_input_state* pState, taUInt32 key)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->keyState[key] & TA_KEY_STATE_IS_DOWN) != 0;
}

taBool32 ta_input_state_was_key_pressed(ta_input_state* pState, taUInt32 key)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->keyState[key] & TA_KEY_STATE_PRESSED) != 0;
}

taBool32 ta_input_state_was_key_released(ta_input_state* pState, taUInt32 key)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->keyState[key] & TA_KEY_STATE_RELEASED) != 0;
}