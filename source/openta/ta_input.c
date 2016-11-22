// Copyright (C) 2016 David Reid. See included LICENSE file.

ta_result ta_input_state_init(ta_input_state* pState)
{
    if (pState == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pState);

    return TA_SUCCESS;
}

ta_result ta_input_state_uninit(ta_input_state* pState)
{
    if (pState == NULL) return TA_INVALID_ARGS;
    return TA_SUCCESS;
}


void ta_input_state_reset_transient_state(ta_input_state* pState)
{
    if (pState == NULL) return;

    // Mouse
    // =====
    for (ta_uint32 i = 0; i < ta_countof(pState->mouseButtonState); ++i) {
        pState->mouseButtonState[i] &= ~TA_MOUSE_BUTTON_STATE_PRESSED;
    }

    pState->mouseOffsetX = 0;
    pState->mouseOffsetY = 0;


    // Keyboard
    // ========
    for (ta_uint32 i = 0; i < ta_countof(pState->keyState); ++i) {
        pState->keyState[i] &= ~TA_KEY_STATE_IS_DOWN;
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

void ta_input_state_on_mouse_button_down(ta_input_state* pState, ta_uint32 mouseButton)
{
    if (pState == NULL) return;
    pState->mouseButtonState[mouseButton] |= (TA_MOUSE_BUTTON_STATE_IS_DOWN | TA_MOUSE_BUTTON_STATE_PRESSED);
}

void ta_input_state_on_mouse_button_up(ta_input_state* pState, ta_uint32 mouseButton)
{
    if (pState == NULL) return;
    pState->mouseButtonState[mouseButton] &= ~(TA_MOUSE_BUTTON_STATE_IS_DOWN);
}

dr_bool32 ta_input_state_is_mouse_button_down(ta_input_state* pState, ta_uint32 mouseButton)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_IS_DOWN) != 0;
}

dr_bool32 ta_input_state_was_mouse_button_pressed(ta_input_state* pState, ta_uint32 mouseButton)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_PRESSED) != 0;
}

dr_bool32 ta_input_state_is_any_mouse_button_down(ta_input_state* pState)
{
    if (pState == NULL) return TA_FALSE;

    for (ta_uint32 i = 0; i < ta_countof(pState->mouseButtonState); ++i) {
        if (ta_input_state_is_mouse_button_down(pState, i)) {
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}



void ta_input_state_on_key_down(ta_input_state* pState, ta_uint32 key)
{
    if (pState == NULL) return;
    pState->keyState[key] |= (TA_KEY_STATE_IS_DOWN | TA_KEY_STATE_PRESSED);
}

void ta_input_state_on_key_up(ta_input_state* pState, ta_uint32 key)
{
    pState->keyState[key] &= ~(TA_KEY_STATE_IS_DOWN);
}

dr_bool32 ta_input_state_is_key_down(ta_input_state* pState, ta_uint32 key)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->keyState[key] & TA_KEY_STATE_IS_DOWN) != 0;
}

dr_bool32 ta_input_state_was_key_pressed(ta_input_state* pState, ta_uint32 key)
{
    if (pState == NULL) return TA_FALSE;
    return (pState->keyState[key] & TA_KEY_STATE_PRESSED) != 0;
}