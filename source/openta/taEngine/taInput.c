// Copyright (C) 2018 David Reid. See included LICENSE file.

taResult taInputStateInit(taInputState* pState)
{
    if (pState == NULL) {
        return TA_INVALID_ARGS;
    }

    taZeroObject(pState);

    return TA_SUCCESS;
}

taResult taInputStateUninit(taInputState* pState)
{
    if (pState == NULL) {
        return TA_INVALID_ARGS;
    }

    return TA_SUCCESS;
}


void taInputStateResetTransientState(taInputState* pState)
{
    if (pState == NULL) {
        return;
    }

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


void taInputStateOnMouseMove(taInputState* pState, float newMousePosX, float newMousePosY)
{
    if (pState == NULL) {
        return;
    }

    pState->mouseOffsetX = newMousePosX - pState->mousePosX;
    pState->mouseOffsetY = newMousePosY - pState->mousePosY;
    pState->mousePosX    = newMousePosX;
    pState->mousePosY    = newMousePosY;
}

void taInputStateOnMouseButtonDown(taInputState* pState, taUInt32 mouseButton)
{
    if (pState == NULL) {
        return;
    }

    pState->mouseButtonState[mouseButton] |= (TA_MOUSE_BUTTON_STATE_IS_DOWN | TA_MOUSE_BUTTON_STATE_PRESSED);
}

void taInputStateOnMouseButtonUp(taInputState* pState, taUInt32 mouseButton)
{
    if (pState == NULL) {
        return;
    }

    pState->mouseButtonState[mouseButton] &= ~(TA_MOUSE_BUTTON_STATE_IS_DOWN);
    pState->mouseButtonState[mouseButton] |= TA_MOUSE_BUTTON_STATE_RELEASED;
}

taBool32 taInputStateIsMouseButtonDown(taInputState* pState, taUInt32 mouseButton)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_IS_DOWN) != 0;
}

taBool32 taInputStateWasMouseButtonPressed(taInputState* pState, taUInt32 mouseButton)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_PRESSED) != 0;
}

taBool32 taInputStateWasMouseButtonReleased(taInputState* pState, taUInt32 mouseButton)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    return (pState->mouseButtonState[mouseButton] & TA_MOUSE_BUTTON_STATE_RELEASED) != 0;
}

taBool32 taInputStateIsAnyMouseButtonDown(taInputState* pState)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    for (taUInt32 i = 0; i < taCountOf(pState->mouseButtonState); ++i) {
        if (taInputStateIsMouseButtonDown(pState, i)) {
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}



void taInputStateOnKeyDown(taInputState* pState, taUInt32 key)
{
    if (pState == NULL) {
        return;
    }

    pState->keyState[key] |= (TA_KEY_STATE_IS_DOWN | TA_KEY_STATE_PRESSED);
}

void taInputStateOnKeyUp(taInputState* pState, taUInt32 key)
{
    pState->keyState[key] &= ~(TA_KEY_STATE_IS_DOWN);
    pState->keyState[key] |= TA_KEY_STATE_RELEASED;
}

taBool32 taInputStateIsKeyDown(taInputState* pState, taUInt32 key)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    return (pState->keyState[key] & TA_KEY_STATE_IS_DOWN) != 0;
}

taBool32 taInputStateWasKeyPressed(taInputState* pState, taUInt32 key)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    return (pState->keyState[key] & TA_KEY_STATE_PRESSED) != 0;
}

taBool32 taInputStateWasKeyReleased(taInputState* pState, taUInt32 key)
{
    if (pState == NULL) {
        return TA_FALSE;
    }

    return (pState->keyState[key] & TA_KEY_STATE_RELEASED) != 0;
}