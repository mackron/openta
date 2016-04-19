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


    // A simple single texture shader.
    GLuint singleTextureShader;


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

struct ta_texture
{
    // The graphics context that owns this texture.
    ta_graphics_context* pGraphics;

    // The OpenGL texture object.
    GLuint objectGL;

    // The width of the texture.
    uint32_t width;

    // The height of the texture.
    uint32_t height;
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


    pGraphics->pGame          = pGame;
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


    // Default state.
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glDisable(GL_DEPTH_TEST);

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


ta_texture* ta_create_texture(ta_graphics_context* pGraphics, unsigned int width, unsigned int height, unsigned int components, const void* pImageData)
{
    if (pGraphics == NULL || width == 0 || height == 0 || (components != 3 && components != 4) || pImageData == NULL) {
        return NULL;
    }

    GLuint objectGL;
    glGenTextures(1, &objectGL);
    
    glBindTexture(GL_TEXTURE_2D, objectGL);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, (components == 3) ? GL_RGB : GL_RGBA, width, height, 0, (components == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pImageData);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);


    // Nearest/Nearest filtering to try and emulate the original feel of the game.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    ta_texture* pTexture = malloc(sizeof(*pTexture));
    if (pTexture == NULL) {
        glDeleteTextures(1, &objectGL);
        return NULL;
    }

    pTexture->pGraphics = pGraphics;
    pTexture->objectGL = objectGL;
    pTexture->width = width;
    pTexture->height = height;

    return pTexture;
}

void ta_delete_texture(ta_texture* pTexture)
{
    glDeleteTextures(1, &pTexture->objectGL);
    free(pTexture);
}




// TESTING
void ta_draw_texture(ta_texture* pTexture)
{
    GLenum error = glGetError();

    // Clear first.
    glClearDepth(1.0);
    glClearColor(0, 0, 0.5, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glViewport(0, 0, pTexture->width, pTexture->height);

    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    
    glBindTexture(GL_TEXTURE_2D, pTexture->objectGL);
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
    }
    glEnd();
}