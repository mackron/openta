// Public domain. See "unlicense" statement at the end of this file.

// REQUIRED HARDWARE REQUIREMENTS
//
// Multitexture (at least 2 textures)
// ARB_vertex_program / ARG_fragment_program
//
//
// OPTIONAL HARDWARE REQUIREMENTS
//
// Vertex Buffer Objects

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

    // The fragment program to use when drawing alpha transparent objects.
    GLuint palettedFragmentProgramTransparent;


    // A mesh for drawing features.
    ta_mesh* pFeaturesMesh;


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

    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLBUFFERDATAPROC glBufferData;


    // Limits.
    GLint maxTextureSize;
    GLboolean supportsVBO;


    // The current resolution.
    GLsizei resolutionX;
    GLsizei resolutionY;

    // The position of the camera.
    int cameraPosX;
    int cameraPosY;


    // Settings.
    bool isShadowsEnabled;


    // State
    ta_vertex_format currentMeshVertexFormat;
    ta_texture* pCurrentTexture;
    ta_mesh* pCurrentMesh;
    GLuint currentFragmentProgram;
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

struct ta_mesh
{
    // The graphics context that owns this mesh.
    ta_graphics_context* pGraphics;

    // The type of primitive making up the mesh.
    GLenum primitiveTypeGL;

    // The format of the meshes vertex data.
    ta_vertex_format vertexFormat;

    // The format of the index data.
    ta_index_format indexFormat;

    // The format of the index data for use by OpenGL.
    GLenum indexFormatGL;


    // A pointer to the vertex data. If VBO's are being used this will be NULL.
    void* pVertexData;

    // A pointer to the index data. If VBO's are being used this will be NULL.
    void* pIndexData;

    // The buffer object for the vertex data. This is 0 if VBO's are not being used.
    GLuint vertexObjectGL;

    // The buffer object for the index data. This is 0 if VBO's are not being used.
    GLuint indexObjectGL;
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

    // Default settings.
    pGraphics->isShadowsEnabled = true;


    // Platform specific.
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

    // VBO
    pGraphics->glGenBuffers = ta_get_gl_proc_address("glGenBuffers");
    pGraphics->glDeleteBuffers = ta_get_gl_proc_address("glDeleteBuffers");
    pGraphics->glBindBuffer = ta_get_gl_proc_address("glBindBuffer");
    pGraphics->glBufferData = ta_get_gl_proc_address("glBufferData");
    if (pGraphics->glGenBuffers == NULL)
    {
        // VBO's aren't supported in core, so try the extension APIs.
        pGraphics->glGenBuffers = ta_get_gl_proc_address("glGenBuffersARB");
        pGraphics->glDeleteBuffers = ta_get_gl_proc_address("glDeleteBuffersARB");
        pGraphics->glBindBuffer = ta_get_gl_proc_address("glBindBufferARB");
        pGraphics->glBufferData = ta_get_gl_proc_address("glBufferDataARB");
    }

    pGraphics->supportsVBO = pGraphics->glGenBuffers != NULL;



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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
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


    pGraphics->glGenProgramsARB(1, &pGraphics->palettedFragmentProgramTransparent);
    pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pGraphics->palettedFragmentProgramTransparent);

    const char palettedFragmentProgramTransparentStr[] =
        "!!ARBfp1.0\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, fragment.texcoord[0], texture[0], 2D;\n"
        "TEMP color;"
        "TEX color, paletteIndex, texture[1], 2D;\n"
        "MUL result.color, color, {1.0, 1.0, 1.0, 0.5};\n"
        "END";
    pGraphics->glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, sizeof(palettedFragmentProgramTransparentStr) - 1, palettedFragmentProgramTransparentStr);    // -1 to remove null terminator.

    errorPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
    if (errorPos != -1)
    {
        // Error.
        printf("--- FRAGMENT SHADER ---\n%s", glGetString(GL_PROGRAM_ERROR_STRING_ARB));
    }



    

    // Default state.
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glClearDepth(1.0f);
    glClearColor(0, 0, 0, 0);


    // Always using vertex and texture coordinate arrays.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // Always using fragment programs.
    glEnable(GL_FRAGMENT_PROGRAM_ARB);



    // Built-in resources.
    uint16_t pFeaturesMeshIndices[4] = {0, 1, 2, 3};
    pGraphics->pFeaturesMesh = ta_create_mutable_mesh(pGraphics, ta_primitive_type_quad, ta_vertex_format_p2t2, 4, NULL, ta_index_format_uint16, 4, pFeaturesMeshIndices);
    if (pGraphics->pFeaturesMesh == NULL) {
        goto on_error;
    }


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

    if (pGraphics->pCurrentTexture) {
        glBindTexture(GL_TEXTURE_2D, pGraphics->pCurrentTexture->objectGL);
    }


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


ta_mesh* ta_create_empty_mesh(ta_graphics_context* pGraphics, ta_primitive_type primitiveType, ta_vertex_format vertexFormat, ta_index_format indexFormat)
{
    if (pGraphics == NULL) {
        return NULL;
    }

    ta_mesh* pMesh = calloc(1, sizeof(*pMesh));
    if (pMesh == NULL) {
        return NULL;
    }

    pMesh->pGraphics = pGraphics;
    pMesh->vertexFormat = vertexFormat;
    pMesh->indexFormat = indexFormat;

    if (primitiveType == ta_primitive_type_point) {
        pMesh->primitiveTypeGL = GL_POINTS;
    } else if (primitiveType == ta_primitive_type_line) {
        pMesh->primitiveTypeGL = GL_LINES;
    } else if (primitiveType == ta_primitive_type_triangle) {
        pMesh->primitiveTypeGL = GL_TRIANGLES;
    } else {
        pMesh->primitiveTypeGL = GL_QUADS;
    }

    if (indexFormat == ta_index_format_uint16) {
        pMesh->indexFormatGL = GL_UNSIGNED_SHORT;
    } else {
        pMesh->indexFormatGL = GL_UNSIGNED_INT;
    }

    return pMesh;
}

ta_mesh* ta_create_mesh(ta_graphics_context* pGraphics, ta_primitive_type primitiveType, ta_vertex_format vertexFormat, uint32_t vertexCount, const void* pVertexData, ta_index_format indexFormat, uint32_t indexCount, const void* pIndexData)
{
    ta_mesh* pMesh = ta_create_empty_mesh(pGraphics, primitiveType, vertexFormat, indexFormat);
    if (pMesh == NULL) {
        return NULL;
    }


    uint32_t vertexBufferSize = vertexCount;
    if (vertexFormat == ta_vertex_format_p2t2) {
        vertexBufferSize *= sizeof(ta_vertex_p2t2);
    } else {
        vertexBufferSize *= sizeof(ta_vertex_p3t2);
    }

    uint32_t indexBufferSize = indexCount * ((uint32_t)indexFormat);


    if (pGraphics->supportsVBO)
    {
        // Use VBO's.
        pMesh->pVertexData = NULL;
        pMesh->pIndexData = NULL;

        GLuint buffers[2];  // 0 = VBO; 1 = IBO.
        pGraphics->glGenBuffers(2, buffers);


        // Vertices.
        pMesh->vertexObjectGL = buffers[0];
        pGraphics->glBindBuffer(GL_ARRAY_BUFFER, pMesh->vertexObjectGL);
        pGraphics->glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, pVertexData, GL_STATIC_DRAW);

        // Indices.
        pMesh->indexObjectGL = buffers[1];
        pGraphics->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexObjectGL);
        pGraphics->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, pIndexData, GL_STATIC_DRAW);
    }
    else
    {
        // Do not use VBO's.
        pMesh->vertexObjectGL = 0;
        pMesh->indexObjectGL = 0;

        pMesh->pVertexData = malloc(vertexBufferSize);
        if (pMesh->pVertexData == NULL) {
            free(pMesh);
            return NULL;
        }

        memcpy(pMesh->pVertexData, pVertexData, vertexBufferSize);


        pMesh->pIndexData = malloc(indexBufferSize);
        if (pMesh->pIndexData == NULL) {
            free(pMesh->pVertexData);
            free(pMesh);
            return NULL;
        }

        memcpy(pMesh->pIndexData, pIndexData, indexBufferSize);
    }

    return pMesh;
}

ta_mesh* ta_create_mutable_mesh(ta_graphics_context* pGraphics, ta_primitive_type primitiveType, ta_vertex_format vertexFormat, uint32_t vertexCount, const void* pVertexData, ta_index_format indexFormat, uint32_t indexCount, const void* pIndexData)
{
    ta_mesh* pMesh = ta_create_empty_mesh(pGraphics, primitiveType, vertexFormat, indexFormat);
    if (pMesh == NULL) {
        return NULL;
    }

    uint32_t vertexBufferSize = vertexCount;
    if (vertexFormat == ta_vertex_format_p2t2) {
        vertexBufferSize *= sizeof(ta_vertex_p2t2);
    } else {
        vertexBufferSize *= sizeof(ta_vertex_p3t2);
    }

    pMesh->pVertexData = malloc(vertexBufferSize);
    if (pMesh->pVertexData == NULL) {
        free(pMesh);
        return NULL;
    }

    if (pVertexData) {
        memcpy(pMesh->pVertexData, pVertexData, vertexBufferSize);
    }


    uint32_t indexBufferSize = indexCount * ((uint32_t)indexFormat);
    pMesh->pIndexData = malloc(indexBufferSize);
    if (pMesh->pIndexData == NULL) {
        free(pMesh->pVertexData);
        free(pMesh);
        return NULL;
    }

    if (pIndexData) {
        memcpy(pMesh->pIndexData, pIndexData, indexBufferSize);
    }


    return pMesh;
}

void ta_delete_mesh(ta_mesh* pMesh)
{
    if (pMesh == NULL) {
        return;
    }

    if (pMesh->vertexObjectGL) {
        pMesh->pGraphics->glDeleteBuffers(1, &pMesh->vertexObjectGL);
    }
    if (pMesh->indexObjectGL) {
        pMesh->pGraphics->glDeleteBuffers(1, &pMesh->indexObjectGL);
    }

    if (pMesh->pVertexData) {
        free(pMesh->pVertexData);
    }
    if (pMesh->pIndexData) {
        free(pMesh->pIndexData);
    }

    free(pMesh);
}



//// Limits ////

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

    //printf("Camera Pos: %d %d\n", pGraphics->cameraPosX, pGraphics->cameraPosY);
}

static TA_INLINE void ta_graphics__bind_texture(ta_graphics_context* pGraphics, ta_texture* pTexture)
{
    assert(pGraphics != NULL);

    if (pGraphics->pCurrentTexture == pTexture) {
        return;
    }


    if (pTexture != NULL) {
        glBindTexture(GL_TEXTURE_2D, pTexture->objectGL);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    pGraphics->pCurrentTexture = pTexture;
}

static TA_INLINE void ta_graphics__bind_fragment_program(ta_graphics_context* pGraphics, GLuint fragmentProgram)
{
    assert(pGraphics != NULL);

    if (pGraphics->currentFragmentProgram == fragmentProgram) {
        return;
    }

    pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fragmentProgram);
    pGraphics->currentFragmentProgram = fragmentProgram;
}

static TA_INLINE void ta_graphics__bind_mesh(ta_graphics_context* pGraphics, ta_mesh* pMesh)
{
    assert(pGraphics != NULL);
    
    if (pGraphics->pCurrentMesh == pMesh) {
        return;
    }

    if (pMesh != NULL)
    {
        if (pMesh->pVertexData != NULL)
        {
            // Using vertex arrays.
            if (pGraphics->supportsVBO) {
                pGraphics->glBindBuffer(GL_ARRAY_BUFFER, 0);
                pGraphics->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }

            if (pMesh->vertexFormat == ta_vertex_format_p2t2) {
                glVertexPointer(2, GL_FLOAT, sizeof(ta_vertex_p2t2), pMesh->pVertexData);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p2t2), ((uint8_t*)pMesh->pVertexData) + (2*sizeof(float)));
            } else {
                glVertexPointer(3, GL_FLOAT, sizeof(ta_vertex_p3t2), pMesh->pVertexData);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p3t2), ((uint8_t*)pMesh->pVertexData) + (3*sizeof(float)));
            }
        }
        else if (pMesh->vertexObjectGL)
        {
            pGraphics->glBindBuffer(GL_ARRAY_BUFFER, pMesh->vertexObjectGL);
            pGraphics->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexObjectGL);

            if (pMesh->vertexFormat == ta_vertex_format_p2t2) {
                glVertexPointer(2, GL_FLOAT, sizeof(ta_vertex_p2t2), 0);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p2t2), (const GLvoid*)(2*sizeof(float)));
            } else {
                glVertexPointer(3, GL_FLOAT, sizeof(ta_vertex_p3t2), 0);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p3t2), (const GLvoid*)(3*sizeof(float)));
            }
        }
    }
    else
    {
        glVertexPointer(4, GL_FLOAT, 0, NULL);
        glTexCoordPointer(4, GL_FLOAT, 0, NULL);

        if (pGraphics->supportsVBO) {
            pGraphics->glBindBuffer(GL_ARRAY_BUFFER, 0);
            pGraphics->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }

    pGraphics->pCurrentMesh = pMesh;
}

static TA_INLINE void ta_graphics__draw_mesh(ta_graphics_context* pGraphics, ta_mesh* pMesh, uint32_t indexCount, uint32_t indexOffset)
{
    // Pre: The mesh is assumed to be bound.
    assert(pGraphics != NULL);
    assert(pGraphics->pCurrentMesh = pMesh);
    assert(pMesh != NULL);

    uint32_t byteOffset = indexOffset * ((uint32_t)pMesh->indexFormat);

    if (pMesh->pIndexData != NULL) {
        glDrawElements(pMesh->primitiveTypeGL, indexCount, pMesh->indexFormatGL, (uint8_t*)pMesh->pIndexData + byteOffset);
    } else {
        glDrawElements(pMesh->primitiveTypeGL, indexCount, pMesh->indexFormatGL, (const GLvoid*)byteOffset);
    }
}

void ta_draw_map_terrain(ta_graphics_context* pGraphics, ta_map_instance* pMap)
{
    // Pre: The paletted fragment program should be bound.
    // Pre: Fragment program should be enabled.

    // The terrain is the base layer so there's no need to clear the color buffer - we just draw over it anyway.
    GLbitfield clearFlags = GL_DEPTH_BUFFER_BIT;
    if (pGraphics->cameraPosX < 0 || (uint32_t)pGraphics->cameraPosX + pGraphics->resolutionX > (pMap->terrain.tileCountX * 32) ||
        pGraphics->cameraPosY < 0 || (uint32_t)pGraphics->cameraPosY + pGraphics->resolutionY > (pMap->terrain.tileCountY * 32)) {
        clearFlags |= GL_COLOR_BUFFER_BIT;
    }

    glClear(clearFlags);


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, 0, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef((GLfloat)-pGraphics->cameraPosX, (GLfloat)-pGraphics->cameraPosY, 0);


    // The terrain will never be transparent.
    glDisable(GL_BLEND);


    //ta_graphics__bind_fragment_program(pGraphics, pGraphics->palettedFragmentProgram);

    // The mesh must be bound before we can draw it.
    ta_graphics__bind_mesh(pGraphics, pMap->terrain.pMesh);


    // Only draw visible chunks.
    int cameraLeft = pGraphics->cameraPosX;
    int cameraTop  = pGraphics->cameraPosY;
    int cameraRight = cameraLeft + pGraphics->resolutionX;
    int cameraBottom = cameraTop + pGraphics->resolutionY;

    int chunkPixelWidth  = TA_TERRAIN_CHUNK_SIZE * 32;
    int chunkPixelHeight = TA_TERRAIN_CHUNK_SIZE * 32;

    // To determine the visible chunks we just need to find the left most and top most chunk. Then we just take the remining space on
    // the screen on each axis to calculate how many visible chunks remain.
    int firstChunkPosX = cameraLeft / chunkPixelWidth;
    if (firstChunkPosX < 0) {
        firstChunkPosX = 0;
    }

    int firstChunkPosY = cameraTop / chunkPixelHeight;
    if (firstChunkPosY < 0) {
        firstChunkPosY = 0;
    }

    int pixelsRemainingX = pGraphics->resolutionX - ((firstChunkPosX*chunkPixelWidth  + chunkPixelWidth)  - cameraLeft);
    int pixelsRemainingY = pGraphics->resolutionY - ((firstChunkPosY*chunkPixelHeight + chunkPixelHeight) - cameraTop);

    int visibleChunkCountX = 0;
    if (cameraRight > 0 && cameraLeft < (int)(pMap->terrain.tileCountX*32)) {
        visibleChunkCountX = 1 + (pixelsRemainingX / chunkPixelWidth) + 1;
        if (firstChunkPosX + visibleChunkCountX > (int)pMap->terrain.chunkCountX) {
            visibleChunkCountX = pMap->terrain.chunkCountX - firstChunkPosX;
        }
    }

    int visibleChunkCountY = 0;
    if (cameraBottom > 0 && cameraTop < (int)(pMap->terrain.tileCountY*32)) {
        visibleChunkCountY = 1 + (pixelsRemainingY / chunkPixelHeight) + 1;
        if (firstChunkPosY + visibleChunkCountY > (int)pMap->terrain.chunkCountY) {
            visibleChunkCountY = pMap->terrain.chunkCountY - firstChunkPosY;
        }
    }

    if (visibleChunkCountX == 0 || visibleChunkCountY == 0) {
        return;
    }


    for (int32_t chunkY = 0; chunkY < visibleChunkCountY; ++chunkY)
    {
        for (int32_t chunkX = 0; chunkX < visibleChunkCountX; ++chunkX)
        {
            ta_map_terrain_chunk* pChunk =  &pMap->terrain.pChunks[((chunkY+firstChunkPosY) * pMap->terrain.chunkCountX) + (chunkX+firstChunkPosX)];
            for (uint32_t iMesh = 0; iMesh < pChunk->meshCount; ++iMesh)
            {
                ta_map_terrain_submesh* pSubmesh = &pChunk->pMeshes[iMesh];
                ta_graphics__bind_texture(pGraphics, pMap->ppTextures[pSubmesh->textureIndex]);
                ta_graphics__draw_mesh(pGraphics, pMap->terrain.pMesh, pSubmesh->indexCount, pSubmesh->indexOffset);
            }
        }
    }
}

void ta_draw_map_feature_sequance(ta_graphics_context* pGraphics, ta_map_instance* pMap, ta_map_feature* pFeature, ta_map_feature_sequence* pSequence, uint32_t frameIndex, bool transparent)
{
    if (pSequence == NULL) {
        return;
    }

    ta_map_feature_frame* pFrame = pSequence->pFrames + frameIndex;

    // The position to draw the feature's graphic depends on it's position in the world and it's offset. Also, the y position
    // needs to be adjusted based on the object's altitude to simulate perspective.
    float posX = pFeature->posX - pFrame->offsetX;
    float posY = pFeature->posY - pFrame->offsetY;
    float posZ = pFeature->posZ;

    // Perspective correction for the height.
    posY -= (int)posZ/2;


    int cameraLeft = pGraphics->cameraPosX;
    int cameraTop  = pGraphics->cameraPosY;
    int cameraRight = cameraLeft + pGraphics->resolutionX;
    int cameraBottom = cameraTop + pGraphics->resolutionY;

    if (posX > cameraRight  || posX + pFrame->width  < cameraLeft ||
        posY > cameraBottom || posY + pFrame->height < cameraTop)
    {
        return;
    }


    if (transparent) {
        ta_graphics__bind_fragment_program(pGraphics, pGraphics->palettedFragmentProgramTransparent);
    } else {
        ta_graphics__bind_fragment_program(pGraphics, pGraphics->palettedFragmentProgram);
    }

    ta_texture* pTexture = pMap->ppTextures[pSequence->pFrames[pFeature->currentFrameIndex].textureIndex];
    ta_graphics__bind_texture(pGraphics, pTexture);


    float uvleft   = (float)pFrame->texturePosX / pTexture->width;
    float uvtop    = (float)pFrame->texturePosY / pTexture->height;
    float uvright  = (float)(pFrame->texturePosX + pFrame->width)  / pTexture->width;
    float uvbottom = (float)(pFrame->texturePosY + pFrame->height) / pTexture->height;

    ta_vertex_p2t2* pVertexData = pGraphics->pFeaturesMesh->pVertexData;
    pVertexData[0].x = posX;                 pVertexData[0].y = posY;                  pVertexData[0].u = uvleft;  pVertexData[0].v = uvtop;
    pVertexData[1].x = posX + pFrame->width; pVertexData[1].y = posY;                  pVertexData[1].u = uvright; pVertexData[1].v = uvtop;
    pVertexData[2].x = posX + pFrame->width; pVertexData[2].y = posY + pFrame->height; pVertexData[2].u = uvright; pVertexData[2].v = uvbottom;
    pVertexData[3].x = posX;                 pVertexData[3].y = posY + pFrame->height; pVertexData[3].u = uvleft;  pVertexData[3].v = uvbottom;
    ta_graphics__bind_mesh(pGraphics, pGraphics->pFeaturesMesh);
    ta_graphics__draw_mesh(pGraphics, pGraphics->pFeaturesMesh, 4, 0);
}

void ta_draw_map(ta_graphics_context* pGraphics, ta_map_instance* pMap)
{
    if (pGraphics == NULL || pMap == NULL) {
        return;
    }

    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    ta_graphics__bind_fragment_program(pGraphics, pGraphics->palettedFragmentProgram);


    // Terrain is always laid down first.
    ta_draw_map_terrain(pGraphics, pMap);


    // Features can be assumed to be transparent.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (uint32_t iFeature = 0; iFeature < pMap->featureCount; ++iFeature)
    {
        ta_map_feature* pFeature = pMap->pFeatures + iFeature;
        if (pFeature->pType->pSequenceDefault)
        {
            // Draw the shadow if we have one.
            if (pFeature->pType->pSequenceShadow != NULL && pGraphics->isShadowsEnabled) {
                ta_draw_map_feature_sequance(pGraphics, pMap, pFeature, pFeature->pType->pSequenceShadow, 0, (pFeature->pType->pDesc->flags & TA_FEATURE_SHADOWTRANSPARENT) != 0);
            }

            if (pFeature->pCurrentSequence != NULL) {
                ta_draw_map_feature_sequance(pGraphics, pMap, pFeature, pFeature->pCurrentSequence, pFeature->currentFrameIndex, false);    // "false" means don't use transparency.
            }
        }
        else
        {
            // The feature has no default sequence which means it's probably a 3D object.

            // TODO: Implement Me.
        }
    }
}


//// Settings ////

void ta_graphics_set_enable_shadows(ta_graphics_context* pGraphics, bool isShadowsEnabled)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->isShadowsEnabled = isShadowsEnabled;
}

bool ta_graphics_get_enable_shadows(ta_graphics_context* pGraphics)
{
    if (pGraphics == NULL) {
        return false;
    }

    return pGraphics->isShadowsEnabled;
}



// TESTING
void ta_draw_subtexture(ta_texture* pTexture, int posX, int posY, bool transparent, int offsetX, int offsetY, int width, int height)
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
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }


    // We need to use a different fragment program depending on whether or not we're using a paletted texture.
    bool isPaletted = pTexture->components == 1;
    if (isPaletted) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ta_graphics__bind_fragment_program(pGraphics, pGraphics->palettedFragmentProgram);
    } else {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }
    

    ta_graphics__bind_texture(pGraphics, pTexture);
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
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    if (transparent) {
        glDisable(GL_BLEND);
    }
}

void ta_draw_texture(ta_texture* pTexture, bool transparent)
{
    ta_draw_subtexture(pTexture, 0, 0, transparent, 0, 0, pTexture->width, pTexture->height);
}