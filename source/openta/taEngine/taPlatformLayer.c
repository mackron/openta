// Copyright (C) 2018 David Reid. See included LICENSE file.

#ifdef _WIN32
static const char* g_TAWndClassName = "TAMainWindow";

#define GET_X_LPARAM(lp)    ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)    ((int)(short)HIWORD(lp))

static void taWin32TrackMouseLeaveEvent(HWND hWnd)
{
    TRACKMOUSEEVENT tme;
    ZeroMemory(&tme, sizeof(tme));
    tme.cbSize    = sizeof(tme);
    tme.dwFlags   = TME_LEAVE;
    tme.hwndTrack = hWnd;
    TrackMouseEvent(&tme);
}

taBool32 taIsWin32MouseButtonKeyCode(WPARAM wParam)
{
    return wParam == VK_LBUTTON || wParam == VK_RBUTTON || wParam == VK_MBUTTON || wParam == VK_XBUTTON1 || wParam == VK_XBUTTON2;
}

taKey taFromWin32Key(WPARAM wParam)
{
    switch (wParam)
    {
    case VK_BACK:   return TA_KEY_BACKSPACE;
    case VK_SHIFT:  return TA_KEY_SHIFT;
    case VK_ESCAPE: return TA_KEY_ESCAPE;
    case VK_PRIOR:  return TA_KEY_PAGE_UP;
    case VK_NEXT:   return TA_KEY_PAGE_DOWN;
    case VK_END:    return TA_KEY_END;
    case VK_HOME:   return TA_KEY_HOME;
    case VK_LEFT:   return TA_KEY_ARROW_LEFT;
    case VK_UP:     return TA_KEY_ARROW_UP;
    case VK_RIGHT:  return TA_KEY_ARROW_RIGHT;
    case VK_DOWN:   return TA_KEY_ARROW_DOWN;
    case VK_DELETE: return TA_KEY_DELETE;

    default: break;
    }

    return (taKey)wParam;
}

unsigned int taWin32GetModifierKeyStateFlags()
{
    unsigned int stateFlags = 0;

    SHORT keyState = GetAsyncKeyState(VK_SHIFT);
    if (keyState & 0x8000) {
        stateFlags |= TA_KEY_STATE_SHIFT_DOWN;
    }

    keyState = GetAsyncKeyState(VK_CONTROL);
    if (keyState & 0x8000) {
        stateFlags |= TA_KEY_STATE_CTRL_DOWN;
    }

    keyState = GetAsyncKeyState(VK_MENU);
    if (keyState & 0x8000) {
        stateFlags |= TA_KEY_STATE_ALT_DOWN;
    }

    return stateFlags;
}

unsigned int taWin32GetMouseEventStateFlags(WPARAM wParam)
{
    unsigned int stateFlags = 0;

    if ((wParam & MK_LBUTTON) != 0) {
        stateFlags |= TA_MOUSE_BUTTON_LEFT_DOWN;
    }

    if ((wParam & MK_RBUTTON) != 0) {
        stateFlags |= TA_MOUSE_BUTTON_RIGHT_DOWN;
    }

    if ((wParam & MK_MBUTTON) != 0) {
        stateFlags |= TA_MOUSE_BUTTON_MIDDLE_DOWN;
    }

    if ((wParam & MK_XBUTTON1) != 0) {
        stateFlags |= TA_MOUSE_BUTTON_4_DOWN;
    }

    if ((wParam & MK_XBUTTON2) != 0) {
        stateFlags |= TA_MOUSE_BUTTON_5_DOWN;
    }


    if ((wParam & MK_CONTROL) != 0) {
        stateFlags |= TA_KEY_STATE_CTRL_DOWN;
    }

    if ((wParam & MK_SHIFT) != 0) {
        stateFlags |= TA_KEY_STATE_SHIFT_DOWN;
    }


    SHORT keyState = GetAsyncKeyState(VK_MENU);
    if (keyState & 0x8000) {
        stateFlags |= TA_KEY_STATE_ALT_DOWN;
    }


    return stateFlags;
}


// This is the event handler for the main game window. The operating system will notify the application of events
// through this function, such as when the mouse is moved over a window, a key is pressed, etc.
static LRESULT DefaultWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    taWindow* pWindow = (taWindow*)GetWindowLongPtrA(hWnd, 0);
    if (pWindow != NULL)
    {
        switch (msg)
        {
            case WM_CREATE:
            {
                // This allows us to track mouse enter and leave events for the window.
                taWin32TrackMouseLeaveEvent(hWnd);
                return 0;
            }

            case WM_CLOSE:
            {
                taPostQuitMessage(0);
                return 0;
            }



            case WM_SIZE:
            {
                taOnWindowSize(pWindow->pEngine, LOWORD(lParam), HIWORD(lParam));
                break;
            }



            case WM_LBUTTONDOWN:
            {
                taOnMouseButtonDown(pWindow->pEngine, TA_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_LEFT_DOWN);
                break;
            }
            case WM_LBUTTONUP:
            {
                taOnMouseButtonUp(pWindow->pEngine, TA_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam));
                break;
            }
            case WM_LBUTTONDBLCLK:
            {
                taOnMouseButtonDown(pWindow->pEngine, TA_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_LEFT_DOWN);
                taOnMouseButtonDblClick(pWindow->pEngine, TA_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_LEFT_DOWN);
                break;
            }


            case WM_RBUTTONDOWN:
            {
                taOnMouseButtonDown(pWindow->pEngine, TA_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_RIGHT_DOWN);
                break;
            }
            case WM_RBUTTONUP:
            {
                taOnMouseButtonUp(pWindow->pEngine, TA_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam));
                break;
            }
            case WM_RBUTTONDBLCLK:
            {
                taOnMouseButtonDown(pWindow->pEngine, TA_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_RIGHT_DOWN);
                taOnMouseButtonDblClick(pWindow->pEngine, TA_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_RIGHT_DOWN);
                break;
            }


            case WM_MBUTTONDOWN:
            {
                taOnMouseButtonDown(pWindow->pEngine, TA_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_MIDDLE_DOWN);
                break;
            }
            case WM_MBUTTONUP:
            {
                taOnMouseButtonUp(pWindow->pEngine, TA_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam));
                break;
            }
            case WM_MBUTTONDBLCLK:
            {
                taOnMouseButtonDown(pWindow->pEngine, TA_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_MIDDLE_DOWN);
                taOnMouseButtonDblClick(pWindow->pEngine, TA_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam) | TA_MOUSE_BUTTON_MIDDLE_DOWN);
                break;
            }


            case WM_MOUSEWHEEL:
            {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

                POINT p;
                p.x = GET_X_LPARAM(lParam);
                p.y = GET_Y_LPARAM(lParam);
                ScreenToClient(hWnd, &p);

                taOnMouseWheel(pWindow->pEngine, delta, p.x, p.y, taWin32GetMouseEventStateFlags(wParam));
                break;
            }


            case WM_MOUSELEAVE:
            {
                pWindow->isCursorOver = TA_FALSE;

                taOnMouseLeave(pWindow->pEngine);
                break;
            }

            case WM_MOUSEMOVE:
            {
                // On Win32 we need to explicitly tell the operating system to post a WM_MOUSELEAVE event. The problem is that it needs to be re-issued when the
                // mouse re-enters the window. The easiest way to do this is to just call it in response to every WM_MOUSEMOVE event.
                if (!pWindow->isCursorOver)
                {
                    taWin32TrackMouseLeaveEvent(hWnd);
                    pWindow->isCursorOver = TA_TRUE;

                    taOnMouseEnter(pWindow->pEngine);
                }

                taOnMouseMove(pWindow->pEngine, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), taWin32GetMouseEventStateFlags(wParam));
                break;
            }


            case WM_KEYDOWN:
            {
                if (!taIsWin32MouseButtonKeyCode(wParam))
                {
                    unsigned int stateFlags = taWin32GetModifierKeyStateFlags();
                    if ((lParam & (1 << 30)) != 0) {
                        stateFlags |= TA_KEY_STATE_AUTO_REPEATED;
                    }

                    taOnKeyDown(pWindow->pEngine, taFromWin32Key(wParam), stateFlags);
                }

                break;
            }

            case WM_KEYUP:
            {
                if (!taIsWin32MouseButtonKeyCode(wParam))
                {
                    unsigned int stateFlags = taWin32GetModifierKeyStateFlags();
                    taOnKeyUp(pWindow->pEngine, taFromWin32Key(wParam), stateFlags);
                }

                break;
            }

            // NOTE: WM_UNICHAR is not posted by Windows itself, but rather intended to be posted by applications. Thus, we need to use WM_CHAR. WM_CHAR
            //       posts events as UTF-16 code points. When the code point is a surrogate pair, we need to store it and wait for the next WM_CHAR event
            //       which will contain the other half of the pair.
            case WM_CHAR:
            {
                // Windows will post WM_CHAR events for keys we don't particularly want. We'll filter them out here (they will be processed by WM_KEYDOWN).
                if (wParam < 32 || wParam == 127)       // 127 = ASCII DEL (will be triggered by CTRL+Backspace)
                {
                    if (wParam != VK_TAB  &&
                        wParam != VK_RETURN)    // VK_RETURN = Enter Key.
                    {
                        break;
                    }
                }


                if ((lParam & (1U << 31)) == 0)     // Bit 31 will be 1 if the key was pressed, 0 if it was released.
                {
                    if (IS_HIGH_SURROGATE(wParam))
                    {
                        assert(pWindow->utf16HighSurrogate == 0);
                        pWindow->utf16HighSurrogate = (unsigned short)wParam;
                    }
                    else
                    {
                        unsigned int character = (unsigned int)wParam;
                        if (IS_LOW_SURROGATE(wParam))
                        {
                            assert(IS_HIGH_SURROGATE(pWindow->utf16HighSurrogate) != 0);
                            character = taUTF16PairToUTF32ch(pWindow->utf16HighSurrogate, (unsigned short)wParam);
                        }

                        pWindow->utf16HighSurrogate = 0;


                        int repeatCount = lParam & 0x0000FFFF;
                        for (int i = 0; i < repeatCount; ++i)
                        {
                            unsigned int stateFlags = taWin32GetModifierKeyStateFlags();
                            if ((lParam & (1 << 30)) != 0) {
                                stateFlags |= TA_KEY_STATE_AUTO_REPEATED;
                            }

                            taOnPrintableKeyDown(pWindow->pEngine, character, stateFlags);
                        }
                    }
                }

                break;
            }


            default: break;
        }
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}


typedef enum TA_PROCESS_DPI_AWARENESS {
    TA_PROCESS_DPI_UNAWARE = 0,
    TA_PROCESS_SYSTEM_DPI_AWARE = 1,
    TA_PROCESS_PER_MONITOR_DPI_AWARE = 2
} OC_PROCESS_DPI_AWARENESS;

typedef enum OC_MONITOR_DPI_TYPE {
    TA_MDT_EFFECTIVE_DPI = 0,
    TA_MDT_ANGULAR_DPI = 1,
    TA_MDT_RAW_DPI = 2,
    TA_MDT_DEFAULT = TA_MDT_EFFECTIVE_DPI
} OC_MONITOR_DPI_TYPE;

typedef BOOL    (__stdcall * OC_PFN_SetProcessDPIAware)     (void);
typedef HRESULT (__stdcall * OC_PFN_SetProcessDpiAwareness) (OC_PROCESS_DPI_AWARENESS);
typedef HRESULT (__stdcall * OC_PFN_GetDpiForMonitor)       (HMONITOR hmonitor, OC_MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);


void taMakeDPIAware_Win32()
{
    taBool32 fallBackToDiscouragedAPI = TA_FALSE;

    // We can't call SetProcessDpiAwareness() directly because otherwise on versions of Windows < 8.1 we'll get an error at load time about
    // a missing DLL.
    HMODULE hSHCoreDLL = LoadLibraryW(L"shcore.dll");
    if (hSHCoreDLL != NULL) {
        OC_PFN_SetProcessDpiAwareness _SetProcessDpiAwareness = (OC_PFN_SetProcessDpiAwareness)GetProcAddress(hSHCoreDLL, "SetProcessDpiAwareness");
        if (_SetProcessDpiAwareness != NULL) {
            if (_SetProcessDpiAwareness(TA_PROCESS_PER_MONITOR_DPI_AWARE) != S_OK) {
                fallBackToDiscouragedAPI = TA_TRUE;
            }
        } else {
            fallBackToDiscouragedAPI = TA_TRUE;
        }

        FreeLibrary(hSHCoreDLL);
    } else {
        fallBackToDiscouragedAPI = TA_TRUE;
    }

    if (fallBackToDiscouragedAPI) {
        HMODULE hUser32DLL = LoadLibraryW(L"user32.dll");
        if (hUser32DLL != NULL) {
            OC_PFN_SetProcessDPIAware _SetProcessDPIAware = (OC_PFN_SetProcessDPIAware)GetProcAddress(hUser32DLL, "SetProcessDPIAware");
            if (_SetProcessDPIAware != NULL) {
                _SetProcessDPIAware();
            }

            FreeLibrary(hUser32DLL);
        }
    }
}


taBool32 taInitWindowSystem()
{
    // The Windows operating system likes to automatically change the size of the game window when DPI scaling is
    // used. For example, if the user has their DPI set to 200%, the operating system will try to be helpful and
    // automatically resize every window by 200%. The size of the window controls the resolution the game runs at,
    // but we want that resolution to be set explicitly via something like an options menu. Thus, we don't want the
    // operating system to be changing the size of the window to anything other than what we explicitly request. To
    // do this, we just tell the operation system that it shouldn't do DPI scaling and that we'll do it ourselves
    // manually.
    taMakeDPIAware_Win32();

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.cbWndExtra    = sizeof(void*);
    wc.lpfnWndProc   = (WNDPROC)DefaultWindowProcWin32;
    wc.lpszClassName = g_TAWndClassName;
    wc.hCursor       = LoadCursorA(NULL, MAKEINTRESOURCEA(32512));
    wc.style         = CS_OWNDC | CS_DBLCLKS;
    if (!RegisterClassExA(&wc)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

void taUninitWindowSystem()
{
    UnregisterClassA(g_TAWndClassName, GetModuleHandleA(NULL));
}

void taPostQuitMessage(int exitCode)
{
    PostQuitMessage(exitCode);
}


taWindow* taCreateWindow(taEngineContext* pEngine, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options)
{
    DWORD dwExStyle = 0;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;

    HWND hWnd = CreateWindowExA(dwExStyle, g_TAWndClassName, pTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, resolutionX, resolutionY, NULL, NULL, NULL, NULL);
    if (hWnd == NULL) {
        return NULL;
    }

    // We should have a window, but before showing it we need to make a few small adjustments to the position and size. First, we need to
    // honour the TA_WINDOW_CENTERED option. Second, we need to make a small change to the size of the window such that the client size
    // is equal to resolutionX and resolutionY. When we created the window, we specified resolutionX and resolutionY as the dimensions,
    // however this includes the size of the outer border. The outer border should not be included, so we need to stretch the window just
    // a little bit such that the area inside the borders are exactly equal to resolutionX and resolutionY.
    //
    // We use a single API to both move and resize the window.

    UINT swpflags = SWP_NOZORDER | SWP_NOMOVE;

    RECT windowRect;
    RECT clientRect;
    GetWindowRect(hWnd, &windowRect);
    GetClientRect(hWnd, &clientRect);

    int windowWidth  = (int)resolutionX + ((windowRect.right - windowRect.left) - (clientRect.right - clientRect.left));
    int windowHeight = (int)resolutionY + ((windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top));

    int windowPosX = 0;
    int windowPosY = 0;
    if ((options & TA_WINDOW_CENTERED) != 0) {
        // We need to center the window. To do this properly, we want to reposition based on the monitor it started on.
        MONITORINFO mi;
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(MONITORINFO);
        if (GetMonitorInfoA(MonitorFromWindow(hWnd, 0), &mi))
        {
            windowPosX = ((mi.rcMonitor.right - mi.rcMonitor.left) - windowWidth)  / 2;
            windowPosY = ((mi.rcMonitor.bottom - mi.rcMonitor.top) - windowHeight) / 2;

            swpflags &= ~SWP_NOMOVE;
        }
    }

    SetWindowPos(hWnd, NULL, windowPosX, windowPosY, windowWidth, windowHeight, swpflags);


    taWindow* pWindow = calloc(1, sizeof(*pWindow));
    if (pWindow == NULL) {
        return NULL;
    }

    pWindow->pEngine = pEngine;
    pWindow->hWnd = hWnd;


    // The window has been position and sized correctly, but now we need to attach our own window object to the internal Win32 object.
    SetWindowLongPtrA(hWnd, 0, (LONG_PTR)pWindow);

    // Finally we can show our window.
    ShowWindow(hWnd, SW_SHOWNORMAL);

    return pWindow;
}

void taDeleteWindow(taWindow* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    DestroyWindow(pWindow->hWnd);
    free(pWindow);
}

void taWindowCaptureMouse(taWindow* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    SetCapture(pWindow->hWnd);
}

void taWindowReleaseMouse()
{
    ReleaseCapture();
}


int taMainLoop(taEngineContext* pEngine)
{
    for (;;) {
        // Handle window events.
        MSG msg;
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return (int)msg.wParam;  // Received a quit message.
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        // After handling the next event in the queue we let the game know it should do the next frame.
        if (pEngine->onStep) {
            pEngine->onStep(pEngine);
        }
    }
}

HDC taGetWindowHDC(taWindow* pWindow)
{
    if (pWindow == NULL) {
        return NULL;
    }

    return GetDC(pWindow->hWnd);
}
#endif

#ifdef __linux__
taBool32 taInitWindowSystem()
{
    // TODO: Implement Me.
    return TA_FALSE;
}

void taUninitWindowSystem()
{
    // TODO: Implement Me.
}

taWindow* taCreateWindow(const char* title, unsigned int width, unsigned int height, unsigned int options)
{
    // TODO: Implement Me.
    return NULL;
}

void taDeleteWindow(taWindow* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    // TODO: Implement Me.

    free(pWindow);
}

int taMainLoop(taEngineContext* pEngine)
{
    // TODO: Implement Me.

    return 0;
}
#endif
