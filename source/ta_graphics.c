// Public domain. See "unlicense" statement at the end of this file.

#include <gl/gl.h>
#include <gl/glext.h>

#ifdef _WIN32
#include <GL/wglext.h>
#endif

struct ta_graphics_context
{
    // A pointer to the game that owns this renderer.
    ta_game* pGame;

    // The current window.
    ta_window* pCurrentWindow;



    // Platform Specific.
#if _WIN32
    // The dummy window for creating the main rendering context.
    HWND hDummyHWND;

    // The dummy DC for creating the main rendering context.
    HDC hDummyDC;

    /// The OpenGL rendering context. One per renderer.
    HGLRC hRC;

    // The pixel format for use with SetPixelFormat()
    int pixelFormat;

    // The pixel format descriptor for use with SetPixelFormat()
    PIXELFORMATDESCRIPTOR pfd;


    // WGL Functions
    PFNWGLSWAPINTERVALEXTPROC SwapIntervalEXT;
#endif
};

#ifdef _WIN32
static LRESULT DummyWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif



ta_graphics_context* ta_create_graphics_context(ta_game* pGame)
{
    if (pGame == NULL) {
        return NULL;
    }

    ta_graphics_context* pGraphics = calloc(1, sizeof(*pGraphics));
    if (pGraphics == NULL) {
        return NULL;
    }


    pGraphics->pGame          = NULL;
    pGraphics->pCurrentWindow = NULL;

#ifdef _WIN32
    pGraphics->hDummyHWND = NULL;
    pGraphics->hDummyDC   = NULL;
    pGraphics->hRC        = NULL;

    WNDCLASSEXW dummyWC;
    memset(&dummyWC, 0, sizeof(dummyWC));
    dummyWC.cbSize        = sizeof(dummyWC);
    dummyWC.lpfnWndProc   = (WNDPROC)DummyWindowProcWin32;
    dummyWC.lpszClassName = L"TA_OpenGL_DummyHWND";
    dummyWC.style         = CS_OWNDC;
    if (!RegisterClassExW(&dummyWC)) {
        goto on_error;
    }

    pGraphics->hDummyHWND = CreateWindowExW(0, L"TA_OpenGL_DummyHWND", L"", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    pGraphics->hDummyDC   = GetDC(pGraphics->hDummyHWND);

    memset(&pGraphics->pfd, 0, sizeof(pGraphics->pfd));
    pGraphics->pfd.nSize        = sizeof(pGraphics->pfd);
    pGraphics->pfd.nVersion     = 1;
    pGraphics->pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pGraphics->pfd.iPixelType   = PFD_TYPE_RGBA;
    pGraphics->pfd.cStencilBits = 8;
    pGraphics->pfd.cDepthBits   = 24;
    pGraphics->pfd.cColorBits   = 32;
    pGraphics->pixelFormat = ChoosePixelFormat(pGraphics->hDummyDC, &pGraphics->pfd);
    if (pGraphics->pixelFormat == 0) {
        goto on_error;
    }

    if (!SetPixelFormat(pGraphics->hDummyDC, pGraphics->pixelFormat,  &pGraphics->pfd)) {
        goto on_error;
    }

    pGraphics->hRC = wglCreateContext(pGraphics->hDummyDC);
    if (pGraphics->hRC == NULL) {
        goto on_error;
    }

    wglMakeCurrent(pGraphics->hDummyDC, pGraphics->hRC);


    // Retrieve WGL function pointers.
    pGraphics->SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
#endif

    return pGraphics;

on_error:
    ta_delete_graphics_context(pGraphics);
    return NULL;
}

void ta_delete_graphics_context(ta_graphics_context* pGraphics)
{
    if (pGraphics == NULL) {
        return;
    }

#ifdef _WIN32
    if (pGraphics->hDummyHWND) {
        DestroyWindow(pGraphics->hDummyHWND);
    }

    if (pGraphics->hRC) {
        wglDeleteContext(pGraphics->hRC);
    }
#endif


    free(pGraphics);
}


ta_window* ta_graphics_create_window(ta_graphics_context* pGraphics, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options)
{
#ifdef _WIN32
    ta_window* pWindow = ta_create_window(pGraphics->pGame, pTitle, resolutionX, resolutionY, options);
    if (pWindow == NULL) {
        return NULL;
    }

    SetPixelFormat(ta_get_window_hdc(pWindow), pGraphics->pixelFormat, &pGraphics->pfd);

    return pWindow;
#endif

    return NULL;
}

void ta_graphics_delete_window(ta_graphics_context* pGraphics, ta_window* pWindow)
{
    (void)pGraphics;
    ta_delete_window(pWindow);
}

void ta_graphics_set_current_window(ta_graphics_context* pGraphics, ta_window* pWindow)
{
    if (pGraphics == NULL || pGraphics->pCurrentWindow == pWindow) {
        return;
    }

#ifdef _WIN32
    wglMakeCurrent(ta_get_window_hdc(pWindow), pGraphics->hRC);
#endif

    pGraphics->pCurrentWindow = pWindow;
}


void ta_graphics_enable_vsync(ta_graphics_context* pGraphics)
{
    if (pGraphics == NULL) {
        return;
    }

    if (pGraphics->SwapIntervalEXT) {
        pGraphics->SwapIntervalEXT(1);
    }
}

void ta_graphics_disable_vsync(ta_graphics_context* pGraphics)
{
    if (pGraphics == NULL) {
        return;
    }

    if (pGraphics->SwapIntervalEXT) {
        pGraphics->SwapIntervalEXT(0);
    }
}

void ta_graphics_present(ta_graphics_context* pGraphics, ta_window* pWindow)
{
    if (pGraphics == NULL || pWindow == NULL) {
        return;
    }

#ifdef _WIN32
    SwapBuffers(ta_get_window_hdc(pWindow));
#endif
}