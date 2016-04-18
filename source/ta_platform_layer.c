// Public domain. See "unlicense" statement at the end of this file.

#ifdef _WIN32
static const char* g_TAWndClassName = "TAMainWindow";

// This is the event handler for the main game window. The operating system will notify the application of events
// through this function, such as when the mouse is moved over a window, a key is pressed, etc.
static LRESULT DefaultWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ta_window* pWindow = (ta_window*)GetWindowLongPtrA(hWnd, 0);
    if (pWindow != NULL)
    {
        switch (msg)
        {
            case WM_CLOSE:
            {
                PostQuitMessage(0);
                return 0;
            }

            default: break;
        }
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

bool ta_init_window_system()
{
    // The Windows operating system likes to automatically change the size of the game window when DPI scaling is
    // used. For example, if the user has their DPI set to 200%, the operating system will try to be helpful and
    // automatically resize every window by 200%. The size of the window controls the resolution the game runs at,
    // but we want that resolution to be set explicitly via something like an options menu. Thus, we don't want the
    // operating system to be changing the size of the window to anything other than what we explicitly request. To
    // do this, we just tell the operation system that it shouldn't do DPI scaling and that we'll do it ourselves
    // manually.
    dr_win32_make_dpi_aware();

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.cbWndExtra    = sizeof(void*);
    wc.lpfnWndProc   = (WNDPROC)DefaultWindowProcWin32;
    wc.lpszClassName = g_TAWndClassName;
    wc.hCursor       = LoadCursorA(NULL, MAKEINTRESOURCEA(32512));
    wc.style         = CS_OWNDC | CS_DBLCLKS;
    if (!RegisterClassExA(&wc)) {
        return false;
    }

    return true;
}

void ta_uninit_window_system()
{
    UnregisterClassA(g_TAWndClassName, GetModuleHandleA(NULL));
}

ta_window* ta_create_window(ta_game* pGame, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options)
{
    DWORD dwExStyle = 0;
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;

    HWND hWnd = CreateWindowExA(dwExStyle, g_TAWndClassName, pTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, resolutionX, resolutionY, NULL, NULL, NULL, NULL);
    if (hWnd == NULL) {
        return NULL;
    }

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);

    windowRect.right = windowRect.left + resolutionX;
    windowRect.bottom = windowRect.top + resolutionY;
    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);


    ta_window* pWindow = calloc(1, sizeof(*pWindow));
    if (pWindow == NULL) {
        return NULL;
    }

    pWindow->pGame = pGame;
    pWindow->hWnd = hWnd;


    // The window has been position and sized correctly, but now we need to attach our own window object to the internal Win32 object.
    SetWindowLongPtrA(hWnd, 0, (LONG_PTR)pWindow);

    // Finally we can show our window.
    ShowWindow(hWnd, SW_SHOWNORMAL);

    return pWindow;
}

void ta_delete_window(ta_window* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    DestroyWindow(pWindow->hWnd);
    free(pWindow);
}

int ta_main_loop(ta_game* pGame)
{
    for (;;)
    {
        // Handle window events.
        MSG msg;
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                return msg.wParam;  // Received a quit message.
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        // After handling the next event in the queue we let the game know it should do the next frame.
        ta_do_frame(pGame);
    }

    return 0;
}

HDC ta_get_window_hdc(ta_window* pWindow)
{
    return GetDC(pWindow->hWnd);
}


struct ta_timer
{
    /// The high performance counter frequency.
    LARGE_INTEGER frequency;

    /// The timer counter at the point in the time the timer was last ticked.
    LARGE_INTEGER counter;
};

ta_timer* ta_create_timer()
{
    ta_timer* pTimer = malloc(sizeof(*pTimer));
    if (pTimer == NULL) {
        return NULL;
    }

    if (!QueryPerformanceFrequency(&pTimer->frequency) || !QueryPerformanceCounter(&pTimer->counter)) {
        free(pTimer);
        return NULL;
    }

    return pTimer;
}

void ta_delete_timer(ta_timer* pTimer)
{
    free(pTimer);
}

double ta_tick_timer(ta_timer* pTimer)
{
    LARGE_INTEGER oldCounter = pTimer->counter;
    if (!QueryPerformanceCounter(&pTimer->counter)) {
        return 0;
    }

    return (pTimer->counter.QuadPart - oldCounter.QuadPart) / (double)pTimer->frequency.QuadPart;
}
#endif

#ifdef __linux__
bool ta_init_window_system()
{
    // TODO: Implement Me.
    return false;
}

void ta_uninit_window_system()
{
    // TODO: Implement Me.
}

ta_window* ta_create_window(const char* title, unsigned int width, unsigned int height, unsigned int options)
{
    // TODO: Implement Me.
    return NULL;
}

void ta_delete_window(ta_window* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    // TODO: Implement Me.

    free(pWindow);
}

int ta_main_loop(ta_game* pGame)
{
    // TODO: Implement Me.

    return 0;
}
#endif
