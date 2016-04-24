// Public domain. See "unlicense" statement at the end of this file.

// HARDWARE REQUIREMENTS
//
// Multitexture (at least 2 textures)
// ARB_vertex_program / ARG_fragment_program

// TODO:
// - Experiment with alpha testing for handling transparency instead of alpha blending.

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


    // The texture containing the palette.
    GLuint paletteTextureGL;

    // The fragment program to use when drawing an object with a paletted texture.
    GLuint palettedFragmentProgram;


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

    PFNGLACTIVETEXTUREPROC glActiveTexture;

    PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
    PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;
    PFNGLBINDPROGRAMARBPROC glBindProgramARB;
    PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
    PFNGLPROGRAMLOCALPARAMETER4FARBPROC glProgramLocalParameter4fARB;


    // Limits.
    GLint maxTextureSize;


    // The current resolution.
    GLsizei resolutionX;
    GLsizei resolutionY;

    // The position of the camera.
    int cameraPosX;
    int cameraPosY;
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

    // The number of components in the texture. If this is set to 1, the texture will be treated as paletted.
    uint32_t components;
};




#ifdef _WIN32
static LRESULT DummyWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif


void* ta_get_gl_proc_address(const char* name)
{
#ifdef _WIN32
    return wglGetProcAddress(name);
#endif
}


ta_graphics_context* ta_create_graphics_context(ta_game* pGame, uint32_t palette[256])
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

    // TODO: Check for support for mandatory extensions such as ARB shaders.

    // Function pointers.
    // Multitexture
    pGraphics->glActiveTexture = ta_get_gl_proc_address("glActiveTexture");

    // ARB_vertex_program / ARG_fragment_program
    pGraphics->glGenProgramsARB = ta_get_gl_proc_address("glGenProgramsARB");
    pGraphics->glDeleteProgramsARB = ta_get_gl_proc_address("glDeleteProgramsARB");
    pGraphics->glBindProgramARB = ta_get_gl_proc_address("glBindProgramARB");
    pGraphics->glProgramStringARB = ta_get_gl_proc_address("glProgramStringARB");
    pGraphics->glProgramLocalParameter4fARB = ta_get_gl_proc_address("glProgramLocalParameter4fARB");


    // Limits.
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &pGraphics->maxTextureSize);


    // The texture containing the palette. This is always bound to texture unit 1.
    pGraphics->glActiveTexture(GL_TEXTURE0 + 1);
    {
        glGenTextures(1, &pGraphics->paletteTextureGL);

        glBindTexture(GL_TEXTURE_2D, pGraphics->paletteTextureGL);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, palette);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    pGraphics->glActiveTexture(GL_TEXTURE0 + 0);
    



    // Create all of the necessary shaders up front.
    pGraphics->glGenProgramsARB(1, &pGraphics->palettedFragmentProgram);
    pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pGraphics->palettedFragmentProgram);

    const char palettedFragmentProgramStr[] =
        "!!ARBfp1.0\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, fragment.texcoord[0], texture[0], 2D;\n"
        "TEX result.color, paletteIndex, texture[1], 2D;\n"
        "END";
    pGraphics->glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, sizeof(palettedFragmentProgramStr) - 1, palettedFragmentProgramStr);    // -1 to remove null terminator.

    GLint errorPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
    if (errorPos != -1)
    {
        // Error.
        printf("--- FRAGMENT SHADER ---\n%s", glGetString(GL_PROGRAM_ERROR_STRING_ARB));
    }



    



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


void ta_graphics_enable_vsync(ta_graphics_context* pGraphics, ta_window* pWindow)
{
    if (pGraphics == NULL) {
        return;
    }

    if (pGraphics->SwapIntervalEXT) {
        ta_window* pPrevWindow = pGraphics->pCurrentWindow;
        ta_graphics_set_current_window(pGraphics, pWindow);

        pGraphics->SwapIntervalEXT(1);

        ta_graphics_set_current_window(pGraphics, pPrevWindow);
    }
}

void ta_graphics_disable_vsync(ta_graphics_context* pGraphics, ta_window* pWindow)
{
    if (pGraphics == NULL) {
        return;
    }

    if (pGraphics->SwapIntervalEXT) {
        ta_window* pPrevWindow = pGraphics->pCurrentWindow;
        ta_graphics_set_current_window(pGraphics, pWindow);

        pGraphics->SwapIntervalEXT(0);

        ta_graphics_set_current_window(pGraphics, pPrevWindow);
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
    if (pGraphics == NULL || width == 0 || height == 0 || (components != 1 && components != 3 && components != 4) || pImageData == NULL) {
        return NULL;
    }

    GLint internalFormat;
    GLenum format;
    switch (components)
    {
        case 1:
        {
            internalFormat = GL_LUMINANCE;
            format = GL_LUMINANCE;
        } break;

        case 3:
        {
            internalFormat = GL_RGB;
            format = GL_RGB;
        } break;

        case 4:
        default:
        {
            internalFormat = GL_RGBA;
            format = GL_RGBA;
        } break;
    }

    GLuint objectGL;
    glGenTextures(1, &objectGL);
    
    glBindTexture(GL_TEXTURE_2D, objectGL);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pImageData);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);


    // Must use nearest/nearest filtering in order for palettes to work properly.
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
    pTexture->components = components;

    return pTexture;
}

void ta_delete_texture(ta_texture* pTexture)
{
    glDeleteTextures(1, &pTexture->objectGL);
    free(pTexture);
}


unsigned int ta_get_max_texture_size(ta_graphics_context* pGraphics)
{
    if (pGraphics == NULL) {
        return 0;
    }

    return (unsigned int)pGraphics->maxTextureSize;
}



//// Drawing ////

void ta_set_resolution(ta_graphics_context* pGraphics, unsigned int resolutionX, unsigned int resolutionY)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->resolutionX = (GLsizei)resolutionX;
    pGraphics->resolutionY = (GLsizei)resolutionY;
    glViewport(0, 0, pGraphics->resolutionX, pGraphics->resolutionY);
}

void ta_set_camera_position(ta_graphics_context* pGraphics, int posX, int posY)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->cameraPosX = posX;
    pGraphics->cameraPosY = posY;
}

void ta_translate_camera(ta_graphics_context* pGraphics, int offsetX, int offsetY)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->cameraPosX += offsetX;
    pGraphics->cameraPosY += offsetY;
}

void ta_draw_terrain(ta_graphics_context* pGraphics, ta_tnt* pTNT)
{
    if (pGraphics == NULL || pTNT == NULL) {
        return;
    }

    // The terrain is the base layer so there's no need to clear the color buffer - we just draw over it anyway.
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, 0, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef((GLfloat)-pGraphics->cameraPosX, (GLfloat)-pGraphics->cameraPosY, 0);

    
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pGraphics->palettedFragmentProgram);


    // TODO: Only draw visible chunks.
    // TODO: Stop using begin/end.
    for (uint32_t iChunk = 0; iChunk < pTNT->chunkCountX*pTNT->chunkCountY; ++iChunk)
    {
        ta_tnt_tile_chunk* pChunk = &pTNT->pChunks[iChunk];
        if (pChunk->subchunkCount > 0)  // ?? Wouldn't have thought this would ever be true, but it is.
        {
            for (uint32_t iSubchunk = 0; iSubchunk < pChunk->subchunkCount; ++iSubchunk)
            {
                glBindTexture(GL_TEXTURE_2D, pChunk->pSubchunks[iSubchunk].pTexture->objectGL);
                glBegin(GL_QUADS);
                {
                    for (uint32_t iVertex = 0; iVertex < pChunk->pSubchunks[iSubchunk].pMesh->quadCount * 4; ++iVertex)
                    {
                        ta_tnt_mesh_vertex* pVertex = pChunk->pSubchunks[iSubchunk].pMesh->pVertices + iVertex;
                        glTexCoord2f(pVertex->u, pVertex->v); glVertex2f(pVertex->x, pVertex->y);
                    }
                }
                glEnd();
            }
        }
    }

    // Restore default state.
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
}



// TESTING
void ta_draw_subtexture(ta_texture* pTexture, bool transparent, int offsetX, int offsetY, int width, int height)
{
    if (pTexture == NULL) {
        return;
    }

    ta_graphics_context* pGraphics = pTexture->pGraphics;

    GLenum error = glGetError();

    // Clear first.
    glClearDepth(1.0);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (transparent) {
        glEnable(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }


    // We need to use a different fragment program depending on whether or not we're using a paletted texture.
    bool isPaletted = pTexture->components == 1;
    if (isPaletted) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pGraphics->palettedFragmentProgram);
    } else {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }
    

    glBindTexture(GL_TEXTURE_2D, pTexture->objectGL);
    glBegin(GL_QUADS);
    {
        float uvleft   = (float)offsetX / pTexture->width;
        float uvtop    = (float)offsetY / pTexture->height;
        float uvright  = (float)(offsetX + width)  / pTexture->width;
        float uvbottom = (float)(offsetY + height) / pTexture->height;

        glTexCoord2f(uvleft, uvbottom); glVertex3f(-1.0f, -1.0f,  1.0f);
        glTexCoord2f(uvright, uvbottom); glVertex3f( 1.0f, -1.0f,  1.0f);
        glTexCoord2f(uvright, uvtop); glVertex3f( 1.0f,  1.0f,  1.0f);
        glTexCoord2f(uvleft, uvtop); glVertex3f(-1.0f,  1.0f,  1.0f);
    }
    glEnd();


    // Restore default state.
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    if (transparent) {
        glDisable(GL_BLEND);
    }
}

void ta_draw_texture(ta_texture* pTexture, bool transparent)
{
    ta_draw_subtexture(pTexture, transparent, 0, 0, pTexture->width, pTexture->height);
}


void ta_draw_tnt_mesh(ta_texture* pTexture, ta_tnt_mesh* pMesh)
{
    if (pTexture == NULL) {
        return;
    }

    ta_graphics_context* pGraphics = pTexture->pGraphics;

    GLenum error = glGetError();

    // Clear first.
    glClearDepth(1.0);
    glClearColor(0, 0, 0.5, 1);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glViewport(0, 0, 640*4, 480*4);

    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glOrtho(0, 640*4, 480*4, 0, 0, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    glDisable(GL_BLEND);


    // We need to use a different fragment program depending on whether or not we're using a paletted texture.
    bool isPaletted = pTexture->components == 1;
    if (isPaletted) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pGraphics->palettedFragmentProgram);
    } else {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }
    

    glBindTexture(GL_TEXTURE_2D, pTexture->objectGL);
    glBegin(GL_QUADS);
    {
#if 1
        for (uint32_t iQuad = 0; iQuad < pMesh->quadCount; ++iQuad)
        {
            ta_tnt_mesh_vertex* pQuad = pMesh->pVertices + (iQuad*4);

            glTexCoord2f(pQuad[0].u, pQuad[0].v); glVertex3f(pQuad[0].x, pQuad[0].y, 0.0f);
            glTexCoord2f(pQuad[1].u, pQuad[1].v); glVertex3f(pQuad[1].x, pQuad[1].y, 0.0f);
            glTexCoord2f(pQuad[2].u, pQuad[2].v); glVertex3f(pQuad[2].x, pQuad[2].y, 0.0f);
            glTexCoord2f(pQuad[3].u, pQuad[3].v); glVertex3f(pQuad[3].x, pQuad[3].y, 0.0f);
        }
#endif

#if 0
        float uvleft   = 0;
        float uvtop    = 0;
        float uvright  = 1;
        float uvbottom = 1;

        glTexCoord2f(0, 0); glVertex3f(0.0f, 0.0f,  0.0f);
        glTexCoord2f(0, 0.03125f); glVertex3f(0.0f, -31.0f,  0.0f);
        glTexCoord2f(0.03125f, 0.03125f); glVertex3f(31.0f,  -31.0f,  0.0f);
        glTexCoord2f(0.03125f, 0.06250f); glVertex3f(31.0f,  0.0f,  0.0f);
#endif
    }
    glEnd();

    glDisable(GL_FRAGMENT_PROGRAM_ARB);

    int a; a = 0;
}
