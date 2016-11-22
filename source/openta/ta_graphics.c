// Copyright (C) 2016 David Reid. See included LICENSE file.

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

typedef struct
{
    GLuint vertexProgram;
    GLuint fragmentProgram;
} ta_graphics_shader;

struct ta_graphics_context
{
    // A pointer to the game that owns this renderer.
    ta_game* pGame;

    // The current window.
    ta_window* pCurrentWindow;


    // The texture containing the palette.
    GLuint paletteTextureGL;

    // The shader to use when drawing an object with a paletted texture.
    ta_graphics_shader palettedShader;

    // The shader to use when drawing alpha transparent objects.
    ta_graphics_shader palettedShaderTransparent;

    // The shader to use when drawing an object with a paletted texture.
    ta_graphics_shader palettedShader3D;

    // The shader to use when drawing text.
    ta_graphics_shader textShader;


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
    ta_bool32 isShadowsEnabled;


    // State
    ta_vertex_format currentMeshVertexFormat;
    ta_texture* pCurrentTexture;
    ta_mesh* pCurrentMesh;
    GLuint currentVertexProgram;
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


// Creates a shader from both a vertex and fragment shader string.
ta_bool32 ta_graphics__compile_shader(ta_graphics_context* pGraphics, ta_graphics_shader* pShader, const char* vertexStr, const char* fragmentStr, char* pOutputLog, size_t outputLogSize)
{
    if (pGraphics == NULL || pShader == NULL) {
        return TA_FALSE;
    }

    // Vertex shader.
    if (vertexStr != NULL) {
        pGraphics->glGenProgramsARB(1, &pShader->vertexProgram);
        pGraphics->glBindProgramARB(GL_VERTEX_PROGRAM_ARB, pShader->vertexProgram);
        pGraphics->glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(vertexStr), vertexStr);    // -1 to remove null terminator.

        GLint errorPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
        if (errorPos != -1) {
            snprintf(pOutputLog, outputLogSize, "--- VERTEX SHADER ---\n%s", glGetString(GL_PROGRAM_ERROR_STRING_ARB));
            return TA_FALSE;
        }
    } else {
        pShader->vertexProgram = 0;
    }


    // Fragment shader.
    if (fragmentStr != NULL) {
        pGraphics->glGenProgramsARB(1, &pShader->fragmentProgram);
        pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pShader->fragmentProgram);
        pGraphics->glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(fragmentStr), fragmentStr);    // -1 to remove null terminator.

        GLint errorPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
        if (errorPos != -1) {
            snprintf(pOutputLog, outputLogSize, "--- FRAGMENT SHADER ---\n%s", glGetString(GL_PROGRAM_ERROR_STRING_ARB));
            return TA_FALSE;
        }
    } else {
        pShader->fragmentProgram = 0;
    }

    return TA_TRUE;
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
    pGraphics->isShadowsEnabled = TA_TRUE;


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
    char shaderOutputLog[4096];

    const char palettedFragmentProgramStr[] =
        "!!ARBfp1.0\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, fragment.texcoord[0], texture[0], 2D;\n"
        "TEX result.color, paletteIndex, texture[1], 2D;\n"
        "END";
    if (!ta_graphics__compile_shader(pGraphics, &pGraphics->palettedShader, NULL, palettedFragmentProgramStr, shaderOutputLog, sizeof(shaderOutputLog))) {
        printf(shaderOutputLog);
    }
    

    const char palettedFragmentProgramTransparentStr[] =
        "!!ARBfp1.0\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, fragment.texcoord[0], texture[0], 2D;\n"
        "TEMP color;"
        "TEX color, paletteIndex, texture[1], 2D;\n"
        "MUL result.color, color, {1.0, 1.0, 1.0, 0.5};\n"
        "END";
    if (!ta_graphics__compile_shader(pGraphics, &pGraphics->palettedShaderTransparent, NULL, palettedFragmentProgramTransparentStr, shaderOutputLog, sizeof(shaderOutputLog))) {
        printf(shaderOutputLog);
    }

    

    const char palettedVertexProgram3DStr[] =
        "!!ARBvp1.0\n"
        "ATTRIB iPos = vertex.position;\n"
        "ATTRIB iTex = vertex.texcoord[0];\n"
        "ATTRIB iNor = vertex.normal;\n"
        "OUTPUT oPos = result.position;\n"
        "OUTPUT oTex = result.texcoord[0];\n"
        "OUTPUT oNor = result.texcoord[1];\n"
        "\n"
        "PARAM mvp[4]  = {state.matrix.mvp};\n"
        "PARAM mvIT[4] = {state.matrix.modelview.invtrans};\n"
        "\n"
        "DP4 oPos.x, vertex.position, mvp[0];\n"
        "DP4 oPos.y, vertex.position, mvp[1];\n"
        "DP4 oPos.z, vertex.position, mvp[2];\n"
        "DP4 oPos.w, vertex.position, mvp[3];\n"
        "\n"
        "MOV oTex, vertex.texcoord[0];\n"
        "\n"
        "DP3 oNor.x, vertex.normal, mvIT[0];\n"     // Inverse-transpose of the model-view matrix.
        "DP3 oNor.y, vertex.normal, mvIT[1];\n"
        "DP3 oNor.z, vertex.normal, mvIT[2];\n"
        "END";

    const char palettedFragmentProgram3DStr[] =
        "!!ARBfp1.0\n"
        "ATTRIB iPos = fragment.position;\n"
        "ATTRIB iTex = fragment.texcoord[0];\n"
        "ATTRIB iNor = fragment.texcoord[1];\n"
        "PARAM L = {0.408248276, -0.408248276, -0.816496551, 1};"
        "\n"
        "TEMP N;"
        "DP3 N.w, iNor, iNor;\n"                      // Normalize
        "RSQ N.w, N.w;\n"
        "MUL N, iNor, N.w;\n"
        "\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, iTex, texture[0], 2D;\n"
        "TEMP color;"
        "TEX color, paletteIndex, texture[1], 2D;\n"
        "\n"
        "TEMP NdotL;\n"
        "DP3 NdotL, N, L;\n"
        "MUL NdotL, NdotL, {1.75, 1.75, 1.75, 1};\n"
        "\n"
        "MUL color, color, NdotL;\n"
        "ADD result.color, color, {0.1, 0.1, 0.1, 0};\n"
        "END";
    if (!ta_graphics__compile_shader(pGraphics, &pGraphics->palettedShader3D, palettedVertexProgram3DStr, palettedFragmentProgram3DStr, shaderOutputLog, sizeof(shaderOutputLog))) {
        printf(shaderOutputLog);
    }


    // For text:
    // - Pixels will be either 0 or 1
    // - When set to 0 it means the pixel is transparent otherwise it's visible.
    // - The shader will subtract 1 from the input pixel to determine whether or not it should be
    //   set to the transparent color.
    // - Local parameter 0 is the text color. Parameter 1 is the transparent color.
    const char textFragmentProgramStr[] =
        "!!ARBfp1.0\n"
        "PARAM textColor = program.local[0];\n"
        "PARAM transparentColor = program.local[1];\n"
        "\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, fragment.texcoord[0], texture[0], 2D;\n"
        "ADD paletteIndex, paletteIndex, {-1, -1, -1, -1};\n"
        "CMP paletteIndex, paletteIndex, transparentColor, textColor;\n"
        "TEX result.color, paletteIndex, texture[1], 2D;\n"
        "END";
    if (!ta_graphics__compile_shader(pGraphics, &pGraphics->textShader, NULL, textFragmentProgramStr, shaderOutputLog, sizeof(shaderOutputLog))) {
        printf(shaderOutputLog);
    }


    // Default state.
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearDepth(1.0f);
    glClearColor(0, 0, 0, 0);


    // Always using vertex and texture coordinate arrays.
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    // Always using fragment programs.
    //glEnable(GL_FRAGMENT_PROGRAM_ARB);



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
    if (components == 1) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

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
    } else if (vertexFormat == ta_vertex_format_p3t2) {
        vertexBufferSize *= sizeof(ta_vertex_p3t2);
    } else {
        vertexBufferSize *= sizeof(ta_vertex_p3t2n3);
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
    } else if (vertexFormat == ta_vertex_format_p3t2) {
        vertexBufferSize *= sizeof(ta_vertex_p3t2);
    } else {
        vertexBufferSize *= sizeof(ta_vertex_p3t2n3);
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

static TA_INLINE void ta_graphics__bind_vertex_program(ta_graphics_context* pGraphics, GLuint vertexProgram)
{
    assert(pGraphics != NULL);

    if (pGraphics->currentVertexProgram == vertexProgram) {
        return;
    }

    if (vertexProgram == 0) {
        glDisable(GL_VERTEX_PROGRAM_ARB);
    } else {
        glEnable(GL_VERTEX_PROGRAM_ARB);
    }

    pGraphics->glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexProgram);
    pGraphics->currentVertexProgram = vertexProgram;
}

static TA_INLINE void ta_graphics__bind_fragment_program(ta_graphics_context* pGraphics, GLuint fragmentProgram)
{
    assert(pGraphics != NULL);

    if (pGraphics->currentFragmentProgram == fragmentProgram) {
        return;
    }

    if (fragmentProgram == 0) {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
    } else {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
    }

    pGraphics->glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fragmentProgram);
    pGraphics->currentFragmentProgram = fragmentProgram;
}

static TA_INLINE void ta_graphics__bind_shader(ta_graphics_context* pGraphics, ta_graphics_shader* pShader)
{
    assert(pGraphics != NULL);

    if (pShader != NULL) {
        ta_graphics__bind_vertex_program(pGraphics, pShader->vertexProgram);
        ta_graphics__bind_fragment_program(pGraphics, pShader->fragmentProgram);
    } else {
        ta_graphics__bind_vertex_program(pGraphics, 0);
        ta_graphics__bind_fragment_program(pGraphics, 0);
    }
}



static TA_INLINE void ta_graphics__bind_mesh(ta_graphics_context* pGraphics, ta_mesh* pMesh)
{
    assert(pGraphics != NULL);
    
    if (pGraphics->pCurrentMesh == pMesh) {
        return;
    }

    glDisableClientState(GL_NORMAL_ARRAY);

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
            } else if (pMesh->vertexFormat == ta_vertex_format_p3t2) {
                glVertexPointer(3, GL_FLOAT, sizeof(ta_vertex_p3t2), pMesh->pVertexData);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p3t2), ((uint8_t*)pMesh->pVertexData) + (3*sizeof(float)));
            } else {
                glEnableClientState(GL_NORMAL_ARRAY);
                glVertexPointer(3, GL_FLOAT, sizeof(ta_vertex_p3t2n3), pMesh->pVertexData);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p3t2n3), ((uint8_t*)pMesh->pVertexData) + (3*sizeof(float)));
                glNormalPointer(GL_FLOAT, sizeof(ta_vertex_p3t2n3), ((uint8_t*)pMesh->pVertexData) + (5*sizeof(float)));
            }
        }
        else if (pMesh->vertexObjectGL)
        {
            pGraphics->glBindBuffer(GL_ARRAY_BUFFER, pMesh->vertexObjectGL);
            pGraphics->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexObjectGL);

            if (pMesh->vertexFormat == ta_vertex_format_p2t2) {
                glVertexPointer(2, GL_FLOAT, sizeof(ta_vertex_p2t2), 0);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p2t2), (const GLvoid*)(2*sizeof(float)));
            } else if (pMesh->vertexFormat == ta_vertex_format_p3t2) {
                glVertexPointer(3, GL_FLOAT, sizeof(ta_vertex_p3t2), 0);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p3t2), (const GLvoid*)(3*sizeof(float)));
            } else {
                glEnableClientState(GL_NORMAL_ARRAY);
                glVertexPointer(3, GL_FLOAT, sizeof(ta_vertex_p3t2n3), 0);
                glTexCoordPointer(2, GL_FLOAT, sizeof(ta_vertex_p3t2n3), (const GLvoid*)(3*sizeof(float)));
                glNormalPointer(GL_FLOAT, sizeof(ta_vertex_p3t2n3), (const GLvoid*)(5*sizeof(float)));
            }
        }
    }
    else
    {
        glVertexPointer(4, GL_FLOAT, 0, NULL);
        glTexCoordPointer(4, GL_FLOAT, 0, NULL);
        glNormalPointer(GL_FLOAT, 0, NULL);

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


void ta_draw_fullscreen_gui(ta_graphics_context* pGraphics, ta_gui* pGUI)
{
    if (pGraphics == NULL || pGUI == NULL) return;

    // Fullscreen GUIs are drawn based on a 640x480 resolution. We want to stretch the GUI, but maintain it's aspect ratio.
    float scale   = 1;
    float offsetX = 0;
    float offsetY = 0;
    ta_gui_get_screen_mapping(pGUI, pGraphics->resolutionX, pGraphics->resolutionY, &scale, &offsetX, &offsetY);

    float quadLeft   = 0;
    float quadTop    = 0;
    float quadRight  = 640;
    float quadBottom = 480;

    quadLeft   *= scale;
    quadTop    *= scale;
    quadRight  *= scale;
    quadBottom *= scale;

    quadLeft   += offsetX;
    quadTop    += offsetY;
    quadRight  += offsetX;
    quadBottom += offsetY;

    // Clear first.
    glClearDepth(1.0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_BLEND);

    if (pGUI->pBackgroundTexture != NULL) {
        ta_graphics__bind_shader(pGraphics, NULL);
        ta_graphics__bind_texture(pGraphics, pGUI->pBackgroundTexture);
        glBegin(GL_QUADS);
        {
            float uvleft   = 0;
            float uvtop    = 0;
            float uvright  = 1;
            float uvbottom = 1;

            glTexCoord2f(uvleft,  uvbottom); glVertex3f(quadLeft,  quadBottom, 0.0f);
            glTexCoord2f(uvright, uvbottom); glVertex3f(quadRight, quadBottom, 0.0f);
            glTexCoord2f(uvright, uvtop);    glVertex3f(quadRight, quadTop,    0.0f);
            glTexCoord2f(uvleft,  uvtop);    glVertex3f(quadLeft,  quadTop,    0.0f);
        }
        glEnd();
    }

    // Gadgets, not including the root.
    for (ta_uint32 iGadget = 1; iGadget < pGUI->gadgetCount; ++iGadget) {   // <-- Start at 1 to skip the root gadget.
        ta_gui_gadget* pGadget = pGUI->pGadgets + iGadget;
        if (pGadget->active == 0) {
            continue;   // Skip over inactive gadgets.
        }

        float posX  = pGadget->xpos   * scale + offsetX;
        float posY  = pGadget->ypos   * scale + offsetY;
        float sizeX = pGadget->width  * scale;
        float sizeY = pGadget->height * scale;
        ta_bool32 isGadgetPressed = pGadget->isHeld && pGUI->hoveredGadgetIndex == iGadget;

        switch (pGadget->id)
        {
            case TA_GUI_GADGET_TYPE_BUTTON:
            {
                if (pGadget->button.pBackgroundTextureGroup != NULL) {
                    ta_uint32 buttonState = (isGadgetPressed) ? TA_GUI_BUTTON_STATE_PRESSED : TA_GUI_BUTTON_STATE_NORMAL;
                    if (pGadget->button.grayedout) {
                        buttonState = TA_GUI_BUTTON_STATE_DISABLED;
                    }

                    ta_gaf_texture_group_frame* pFrame = pGadget->button.pBackgroundTextureGroup->pFrames + pGadget->button.iBackgroundFrame + buttonState;
                    ta_texture* pBackgroundTexture = pGadget->button.pBackgroundTextureGroup->ppAtlases[pFrame->atlasIndex];
                    ta_draw_subtexture(pBackgroundTexture, posX, posY, pFrame->sizeX*scale, pFrame->sizeY*scale, TA_FALSE, pFrame->atlasPosX, pFrame->atlasPosY, pFrame->sizeX, pFrame->sizeY);
                }

                if (!ta_is_string_null_or_empty(pGadget->button.text)) {
                    float textSizeX;
                    float textSizeY;
                    ta_font_measure_text(&pGraphics->pGame->font, scale, pGadget->button.text, &textSizeX, &textSizeY);

                    float textPosX = posX + (sizeX - textSizeX)/2;
                    float textPosY = /*posY + (sizeY - textSizeY)/2;*/ posY - (3*scale);    // <-- Should probably improve this a bit.
                    if (isGadgetPressed) {
                        textPosX += 1*scale;
                        textPosY += 1*scale;
                    }

                    ta_draw_text(pGraphics, &pGraphics->pGame->font, 255, scale, textPosX, textPosY, pGadget->button.text);

                    if (pGadget->button.quickkey != 0) {
                        float charPosX;
                        float charPosY;
                        float charSizeX;
                        float charSizeY;
                        if (ta_font_find_character_metrics(&pGraphics->pGame->font, scale, pGadget->button.text, pGadget->button.quickkey, &charPosX, &charPosY, &charSizeX, &charSizeY) == TA_SUCCESS) {
                            float underlineHeight = roundf(1*scale);
                            float underlineOffsetY = roundf(0*scale);
                            charPosX += textPosX;
                            charPosY += textPosY;

                            ta_uint32 underlineRGBA = (isGadgetPressed) ? pGraphics->pGame->palette[0] : pGraphics->pGame->palette[2];
                            float underlineR = ((underlineRGBA & 0x00FF0000) >> 16) / 255.0f;
                            float underlineG = ((underlineRGBA & 0x0000FF00) >>  8) / 255.0f;
                            float underlineB = ((underlineRGBA & 0x000000FF) >>  0) / 255.0f;

                            ta_graphics__bind_shader(pGraphics, NULL);
                            ta_graphics__bind_texture(pGraphics, NULL);
                            glBegin(GL_QUADS);
                            {
                                glColor3f(underlineR, underlineG, underlineB); glVertex3f(charPosX,           charPosY+charSizeY+underlineOffsetY+underlineHeight, 0.0f);
                                glColor3f(underlineR, underlineG, underlineB); glVertex3f(charPosX+charSizeX, charPosY+charSizeY+underlineOffsetY+underlineHeight, 0.0f);
                                glColor3f(underlineR, underlineG, underlineB); glVertex3f(charPosX+charSizeX, charPosY+charSizeY+underlineOffsetY,                 0.0f);
                                glColor3f(underlineR, underlineG, underlineB); glVertex3f(charPosX,           charPosY+charSizeY+underlineOffsetY,                 0.0f);
                                glColor3f(1, 1, 1);
                            }
                            glEnd();
                        }
                    }
                }
            } break;

            default: break;
        }
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
    glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef((GLfloat)-pGraphics->cameraPosX, (GLfloat)-pGraphics->cameraPosY, 0);


    // The terrain will never be transparent.
    glDisable(GL_BLEND);


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

void ta_draw_map_feature_sequance(ta_graphics_context* pGraphics, ta_map_instance* pMap, ta_map_feature* pFeature, ta_map_feature_sequence* pSequence, uint32_t frameIndex, ta_bool32 transparent)
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
        ta_graphics__bind_shader(pGraphics, &pGraphics->palettedShaderTransparent);
    } else {
        ta_graphics__bind_shader(pGraphics, &pGraphics->palettedShader);
    }

    ta_texture* pTexture = pMap->ppTextures[pSequence->pFrames[pFeature->currentFrameIndex].textureIndex];
    ta_graphics__bind_texture(pGraphics, pTexture);


    float uvleft   = (float)pFrame->texturePosX / pTexture->width;
    float uvtop    = (float)pFrame->texturePosY / pTexture->height;
    float uvright  = (float)(pFrame->texturePosX + pFrame->width)  / pTexture->width;
    float uvbottom = (float)(pFrame->texturePosY + pFrame->height) / pTexture->height;

    ta_vertex_p2t2* pVertexData = pGraphics->pFeaturesMesh->pVertexData;
    pVertexData[0].x = posX;                 pVertexData[0].y = posY;                  pVertexData[0].u = uvleft;  pVertexData[0].v = uvtop;
    pVertexData[1].x = posX;                 pVertexData[1].y = posY + pFrame->height; pVertexData[1].u = uvleft;  pVertexData[1].v = uvbottom;
    pVertexData[2].x = posX + pFrame->width; pVertexData[2].y = posY + pFrame->height; pVertexData[2].u = uvright; pVertexData[2].v = uvbottom;
    pVertexData[3].x = posX + pFrame->width; pVertexData[3].y = posY;                  pVertexData[3].u = uvright; pVertexData[3].v = uvtop;
    ta_graphics__bind_mesh(pGraphics, pGraphics->pFeaturesMesh);
    ta_graphics__draw_mesh(pGraphics, pGraphics->pFeaturesMesh, 4, 0);
}

void ta_draw_map_feature_3do_object_recursive(ta_graphics_context* pGraphics, ta_map_instance* pMap, ta_map_feature* pFeature, ta_map_3do* p3DO, uint32_t objectIndex)
{
    ta_map_3do_object* pObject = &p3DO->pObjects[objectIndex];
    assert(pObject != NULL);

    glPushMatrix();
    glTranslatef((float)pObject->relativePosX, (float)pObject->relativePosY, (float)pObject->relativePosZ);
    {
        ta_graphics__bind_shader(pGraphics, &pGraphics->palettedShader3D);

        for (size_t iMesh = 0; iMesh < pObject->meshCount; ++iMesh) {
            ta_map_3do_mesh* p3DOMesh = &p3DO->pMeshes[pObject->firstMeshIndex + iMesh];

            ta_graphics__bind_texture(pGraphics, pMap->ppTextures[p3DOMesh->textureIndex]);
            ta_graphics__bind_mesh(pGraphics, p3DOMesh->pMesh);

            ta_graphics__draw_mesh(pGraphics, p3DOMesh->pMesh, p3DOMesh->indexCount, p3DOMesh->indexOffset);
        }


        // Children.
        if (pObject->firstChildIndex != 0) {
            ta_draw_map_feature_3do_object_recursive(pGraphics, pMap, pFeature, p3DO, pObject->firstChildIndex);
        }
    }
    glPopMatrix();

    // Siblings.
    if (pObject->nextSiblingIndex) {
        ta_draw_map_feature_3do_object_recursive(pGraphics, pMap, pFeature, p3DO, pObject->nextSiblingIndex);
    }
}

void ta_draw_map_feature_3do(ta_graphics_context* pGraphics, ta_map_instance* pMap, ta_map_feature* pFeature, ta_map_3do* p3DO)
{
    assert(pGraphics != NULL);
    assert(pMap != NULL);
    assert(p3DO != NULL);

    float posX = pFeature->posX;
    float posY = pFeature->posY;
    float posZ = pFeature->posZ;

    // Perspective correction for the height.
    posY -= (int)posZ/2;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(posX, posY, posZ);
    glScalef(1, 1, 1);
    glRotatef(27.67f, 1, 0, 0);
    {
        // This disable/enable pair can be made more efficient. Consider splitting 2D and 3D objects and render in separate loops.
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        ta_draw_map_feature_3do_object_recursive(pGraphics, pMap, pFeature, p3DO, 0);
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    }
    glPopMatrix();
}

void ta_draw_map(ta_graphics_context* pGraphics, ta_map_instance* pMap)
{
    if (pGraphics == NULL || pMap == NULL) {
        return;
    }

    ta_graphics__bind_shader(pGraphics, &pGraphics->palettedShader);


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
                ta_draw_map_feature_sequance(pGraphics, pMap, pFeature, pFeature->pCurrentSequence, pFeature->currentFrameIndex, TA_FALSE);    // "TA_FALSE" means don't use transparency.
            }
        }
        else
        {
            // The feature has no default sequence which means it's probably a 3D object.
            if (pFeature->pType->p3DO != NULL) {
                ta_draw_map_feature_3do(pGraphics, pMap, pFeature, pFeature->pType->p3DO);
            }
        }
    }
}

void ta_draw_text(ta_graphics_context* pGraphics, ta_font* pFont, ta_uint8 colorIndex, float scale, float posX, float posY, const char* text)
{
    if (pGraphics == NULL || pFont == NULL || text == NULL) {
        return;
    }

    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    if (pFont->canBeColored) {
        ta_graphics__bind_shader(pGraphics, &pGraphics->textShader);
        pGraphics->glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, colorIndex/255.0f, colorIndex/255.0f, colorIndex/255.0f, colorIndex/255.0f);
        pGraphics->glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, TA_TRANSPARENT_COLOR/255.0f, TA_TRANSPARENT_COLOR/255.0f, TA_TRANSPARENT_COLOR/255.0f, TA_TRANSPARENT_COLOR/255.0f);
    } else {
        ta_graphics__bind_shader(pGraphics, &pGraphics->palettedShader);
    }

    ta_graphics__bind_texture(pGraphics, pFont->pTexture);

    // Immediate mode will do for now, but might want to use vertex arrays later.
    float penPosX = posX;
    float penPosY = posY;

    glBegin(GL_QUADS);
    for (;;) {
        unsigned char c = (unsigned char)*text++;
        if (c == '\0') {
            break;
        }

        ta_font_glyph glyph = pFont->glyphs[c];
        float glyphSizeX = glyph.sizeX;
        float glyphSizeY = glyph.sizeY;
        float glyphPosX  = penPosX + glyph.originX*scale;
        float glyphPosY  = penPosY + glyph.originY*scale;

        float uvleft   = (float)glyph.u;
        float uvtop    = (float)glyph.v;
        float uvright  = uvleft + (glyphSizeX / pFont->pTexture->width);
        float uvbottom = uvtop  + (glyphSizeY / pFont->pTexture->height);

        glTexCoord2f(uvleft,  uvbottom); glVertex3f(glyphPosX,                    glyphPosY + glyphSizeY*scale, 0.0f);
        glTexCoord2f(uvright, uvbottom); glVertex3f(glyphPosX + glyphSizeX*scale, glyphPosY + glyphSizeY*scale, 0.0f);
        glTexCoord2f(uvright, uvtop);    glVertex3f(glyphPosX + glyphSizeX*scale, glyphPosY,                    0.0f);
        glTexCoord2f(uvleft,  uvtop);    glVertex3f(glyphPosX,                    glyphPosY,                    0.0f);
        
        penPosX += glyphSizeX*scale;
        if (c == '\n') {
            penPosY += pFont->height*scale;
            penPosX  = posX;
        }
    }
    glEnd();
}

void ta_draw_textf(ta_graphics_context* pGraphics, ta_font* pFont, ta_uint8 colorIndex, float scale, float posX, float posY, const char* text, ...)
{
    va_list args;
    va_start(args, text);
    {
        char* formattedText = ta_make_stringv(text, args);
        if (formattedText) {
            ta_draw_text(pGraphics, pFont, colorIndex, scale, posX, posY, formattedText);
            ta_free_string(formattedText);
        }
    }
    va_end(args);
}

void ta_draw_subtexture(ta_texture* pTexture, float posX, float posY, float width, float height, ta_bool32 transparent, float subtexturePosX, float subtexturePosY, float subtextureSizeX, float subtextureSizeY)
{
    if (pTexture == NULL) {
        return;
    }

    ta_graphics_context* pGraphics = pTexture->pGraphics;

    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (transparent) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }


    // We need to use a different fragment program depending on whether or not we're using a paletted texture.
    ta_bool32 isPaletted = pTexture->components == 1;
    if (isPaletted) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        ta_graphics__bind_shader(pGraphics, &pGraphics->palettedShader);
    } else {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }
    

    ta_graphics__bind_texture(pGraphics, pTexture);
    glBegin(GL_QUADS);
    {
        float uvleft   = subtexturePosX / pTexture->width;
        float uvtop    = subtexturePosY / pTexture->height;
        float uvright  = (subtexturePosX + subtextureSizeX) / pTexture->width;
        float uvbottom = (subtexturePosY + subtextureSizeY) / pTexture->height;

        glTexCoord2f(uvleft,  uvbottom); glVertex3f(posX,         posY + height, 1.0f);
        glTexCoord2f(uvright, uvbottom); glVertex3f(posX + width, posY + height, 1.0f);
        glTexCoord2f(uvright, uvtop);    glVertex3f(posX + width, posY,          1.0f);
        glTexCoord2f(uvleft,  uvtop);    glVertex3f(posX,         posY,          1.0f);
    }
    glEnd();


    // Restore default state.
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    if (transparent) {
        glDisable(GL_BLEND);
    }
}


//// Settings ////

void ta_graphics_set_enable_shadows(ta_graphics_context* pGraphics, ta_bool32 isShadowsEnabled)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->isShadowsEnabled = isShadowsEnabled;
}

ta_bool32 ta_graphics_get_enable_shadows(ta_graphics_context* pGraphics)
{
    if (pGraphics == NULL) {
        return TA_FALSE;
    }

    return pGraphics->isShadowsEnabled;
}



// TESTING
void ta_draw_texture(ta_texture* pTexture, ta_bool32 transparent)
{
    ta_draw_subtexture(pTexture, 0, 0, (float)pTexture->width, (float)pTexture->height, transparent, 0, 0, (float)pTexture->width, (float)pTexture->height);
}