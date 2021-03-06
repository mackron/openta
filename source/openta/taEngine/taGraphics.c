// Copyright (C) 2018 David Reid. See included LICENSE file.

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

#define GLBIND_IMPLEMENTATION
#include "../../external/glbind/glbind.h"

typedef struct
{
    GLuint vertexProgram;
    GLuint fragmentProgram;
} taGraphicsShader;

struct taGraphicsContext
{
    GLBapi gl;

    // A pointer to the engine context that owns this renderer.
    taEngineContext* pEngine;

    // The current window.
    taWindow* pCurrentWindow;


    // The texture containing the palette.
    GLuint paletteTextureGL;

    // The shader to use when drawing an object with a paletted texture.
    taGraphicsShader palettedShader;

    // The shader to use when drawing alpha transparent objects.
    taGraphicsShader palettedShaderTransparent;

    // The shader to use when drawing an object with a paletted texture.
    taGraphicsShader palettedShader3D;

    // The shader to use when drawing text.
    taGraphicsShader textShader;


    // A mesh for drawing features.
    taMesh* pFeaturesMesh;


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
    taBool32 isShadowsEnabled;


    // State
    taVertexFormat currentMeshVertexFormat;
    taTexture* pCurrentTexture;
    taMesh* pCurrentMesh;
    GLuint currentVertexProgram;
    GLuint currentFragmentProgram;
};

struct taTexture
{
    // The graphics context that owns this texture.
    taGraphicsContext* pGraphics;

    // The OpenGL texture object.
    GLuint objectGL;

    // The width of the texture.
    taUInt32 width;

    // The height of the texture.
    taUInt32 height;

    // The number of components in the texture. If this is set to 1, the texture will be treated as paletted.
    taUInt32 components;
};

struct taMesh
{
    // The graphics context that owns this mesh.
    taGraphicsContext* pGraphics;

    // The type of primitive making up the mesh.
    GLenum primitiveTypeGL;

    // The format of the meshes vertex data.
    taVertexFormat vertexFormat;

    // The format of the index data.
    taIndexFormat indexFormat;

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


taProc taGetGLProcAddress(const char* name)
{
#ifdef _WIN32
    return (taProc)wglGetProcAddress(name);
#endif
}


// Creates a shader from both a vertex and fragment shader string.
TA_PRIVATE taBool32 taGraphicsCompileShader(taGraphicsContext* pGraphics, taGraphicsShader* pShader, const char* vertexStr, const char* fragmentStr, char* pOutputLog, size_t outputLogSize)
{
    if (pGraphics == NULL || pShader == NULL) {
        return TA_FALSE;
    }

    // Vertex shader.
    if (vertexStr != NULL) {
        pGraphics->gl.glGenProgramsARB(1, &pShader->vertexProgram);
        pGraphics->gl.glBindProgramARB(GL_VERTEX_PROGRAM_ARB, pShader->vertexProgram);
        pGraphics->gl.glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(vertexStr), vertexStr);    // -1 to remove null terminator.

        GLint errorPos;
        pGraphics->gl.glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
        if (errorPos != -1) {
            snprintf(pOutputLog, outputLogSize, "--- VERTEX SHADER ---\n%s", pGraphics->gl.glGetString(GL_PROGRAM_ERROR_STRING_ARB));
            return TA_FALSE;
        }
    } else {
        pShader->vertexProgram = 0;
    }


    // Fragment shader.
    if (fragmentStr != NULL) {
        pGraphics->gl.glGenProgramsARB(1, &pShader->fragmentProgram);
        pGraphics->gl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pShader->fragmentProgram);
        pGraphics->gl.glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(fragmentStr), fragmentStr);    // -1 to remove null terminator.

        GLint errorPos;
        pGraphics->gl.glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
        if (errorPos != -1) {
            snprintf(pOutputLog, outputLogSize, "--- FRAGMENT SHADER ---\n%s", pGraphics->gl.glGetString(GL_PROGRAM_ERROR_STRING_ARB));
            return TA_FALSE;
        }
    } else {
        pShader->fragmentProgram = 0;
    }

    return TA_TRUE;
}


taGraphicsContext* taCreateGraphicsContext(taEngineContext* pEngine, taUInt32 palette[256])
{
    if (pEngine == NULL) {
        return NULL;
    }

    taGraphicsContext* pGraphics = calloc(1, sizeof(*pGraphics));
    if (pGraphics == NULL) {
        return NULL;
    }

    // Initialize OpenGL to begin with.
    GLenum resultGL = glbInit(&pGraphics->gl, NULL);
    if (resultGL != GL_NO_ERROR) {
        free(pGraphics);
        return NULL;
    }


    pGraphics->pEngine        = pEngine;
    pGraphics->pCurrentWindow = NULL;

    // Default settings.
    pGraphics->isShadowsEnabled = TA_TRUE;

    // TODO: Check for support for mandatory extensions such as ARB shaders.

    pGraphics->supportsVBO = pGraphics->gl.glGenBuffers != NULL;


    // Limits.
    pGraphics->gl.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &pGraphics->maxTextureSize);


    // The texture containing the palette. This is always bound to texture unit 1.
    pGraphics->gl.glActiveTexture(GL_TEXTURE0 + 1);
    {
        pGraphics->gl.glGenTextures(1, &pGraphics->paletteTextureGL);

        pGraphics->gl.glBindTexture(GL_TEXTURE_2D, pGraphics->paletteTextureGL);
        pGraphics->gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, palette);

        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    pGraphics->gl.glActiveTexture(GL_TEXTURE0 + 0);
    



    // Create all of the necessary shaders up front.
    char shaderOutputLog[4096];

    const char palettedFragmentProgramStr[] =
        "!!ARBfp1.0\n"
        "TEMP paletteIndex;\n"
        "TEX paletteIndex, fragment.texcoord[0], texture[0], 2D;\n"
        "TEX result.color, paletteIndex, texture[1], 2D;\n"
        "END";
    if (!taGraphicsCompileShader(pGraphics, &pGraphics->palettedShader, NULL, palettedFragmentProgramStr, shaderOutputLog, sizeof(shaderOutputLog))) {
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
    if (!taGraphicsCompileShader(pGraphics, &pGraphics->palettedShaderTransparent, NULL, palettedFragmentProgramTransparentStr, shaderOutputLog, sizeof(shaderOutputLog))) {
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
    if (!taGraphicsCompileShader(pGraphics, &pGraphics->palettedShader3D, palettedVertexProgram3DStr, palettedFragmentProgram3DStr, shaderOutputLog, sizeof(shaderOutputLog))) {
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
    if (!taGraphicsCompileShader(pGraphics, &pGraphics->textShader, NULL, textFragmentProgramStr, shaderOutputLog, sizeof(shaderOutputLog))) {
        printf(shaderOutputLog);
    }


    // Default state.
    pGraphics->gl.glEnable(GL_TEXTURE_2D);
    pGraphics->gl.glShadeModel(GL_SMOOTH);
    pGraphics->gl.glDisable(GL_DEPTH_TEST);
    pGraphics->gl.glDepthFunc(GL_LEQUAL);
    pGraphics->gl.glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    pGraphics->gl.glEnable(GL_CULL_FACE);
    pGraphics->gl.glCullFace(GL_BACK);

    pGraphics->gl.glClearDepth(1.0f);
    pGraphics->gl.glClearColor(0, 0, 0, 0);


    // Always using vertex and texture coordinate arrays.
    pGraphics->gl.glEnableClientState(GL_VERTEX_ARRAY);
    pGraphics->gl.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    pGraphics->gl.glEnableClientState(GL_NORMAL_ARRAY);
    
    // Always using fragment programs.
    //glEnable(GL_FRAGMENT_PROGRAM_ARB);



    // Built-in resources.
    taUInt16 pFeaturesMeshIndices[4] = {0, 1, 2, 3};
    pGraphics->pFeaturesMesh = taCreateMutableMesh(pGraphics, taPrimitiveTypeQuad, taVertexFormatP2T2, 4, NULL, taIndexFormatUInt16, 4, pFeaturesMeshIndices);
    if (pGraphics->pFeaturesMesh == NULL) {
        goto on_error;
    }


    return pGraphics;

on_error:
    taDeleteGraphicsContext(pGraphics);
    return NULL;
}

void taDeleteGraphicsContext(taGraphicsContext* pGraphics)
{
    if (pGraphics == NULL) {
        return;
    }

    glbUninit();
    free(pGraphics);
}


taWindow* taGraphicsCreateWindow(taGraphicsContext* pGraphics, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options)
{
#ifdef _WIN32
    taWindow* pWindow = taCreateWindow(pGraphics->pEngine, pTitle, resolutionX, resolutionY, options);
    if (pWindow == NULL) {
        return NULL;
    }

    SetPixelFormat(taGetWindowHDC(pWindow), glbGetPixelFormat(), glbGetPFD());

    return pWindow;
#else
    return NULL;
#endif
}

void taGraphicsDeleteWindow(taGraphicsContext* pGraphics, taWindow* pWindow)
{
    (void)pGraphics;
    taDeleteWindow(pWindow);
}

void taGraphicsSetCurrentWindow(taGraphicsContext* pGraphics, taWindow* pWindow)
{
    if (pGraphics == NULL || pGraphics->pCurrentWindow == pWindow) {
        return;
    }

#ifdef _WIN32
    wglMakeCurrent(taGetWindowHDC(pWindow), glbGetRC());
#endif

    pGraphics->pCurrentWindow = pWindow;
}


void taGraphicsSetSwapInterval(taGraphicsContext* pGraphics, taWindow* pWindow, int interval)
{
    assert(pGraphics != NULL);

#if defined(GLBIND_WGL)
    if (pGraphics->gl.wglSwapIntervalEXT == NULL) {
        return;
    }
#endif
#if defined(GLBIND_GLX)
    if (pGraphics->gl.glXSwapIntervalEXT == NULL) {
        return;
    }
#endif

    taWindow* pPrevWindow = pGraphics->pCurrentWindow;
    taGraphicsSetCurrentWindow(pGraphics, pWindow);

#if defined(GLBIND_WGL)
    pGraphics->gl.wglSwapIntervalEXT(interval);
#endif
#if defined(GLBIND_GLX)
    pGraphics->gl.glXSwapIntervalEXT(taWindowGetDisplay(pWindow), taWindowGetXWindow(pWindow), interval);
#endif

    taGraphicsSetCurrentWindow(pGraphics, pPrevWindow);
}

void taGraphicsEnableVSync(taGraphicsContext* pGraphics, taWindow* pWindow)
{
    if (pGraphics == NULL) {
        return;
    }

    taGraphicsSetSwapInterval(pGraphics, pWindow, 1);
}

void taGraphicsDisableVSync(taGraphicsContext* pGraphics, taWindow* pWindow)
{
    if (pGraphics == NULL) {
        return;
    }

    taGraphicsSetSwapInterval(pGraphics, pWindow, 0);
}

void taGraphicsPresent(taGraphicsContext* pGraphics, taWindow* pWindow)
{
    if (pGraphics == NULL || pWindow == NULL) {
        return;
    }

#ifdef _WIN32
    SwapBuffers(taGetWindowHDC(pWindow));
#endif
}


taTexture* taCreateTexture(taGraphicsContext* pGraphics, unsigned int width, unsigned int height, unsigned int components, const void* pImageData)
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
    pGraphics->gl.glGenTextures(1, &objectGL);
    
    pGraphics->gl.glBindTexture(GL_TEXTURE_2D, objectGL);

    pGraphics->gl.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pGraphics->gl.glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, pImageData);
    pGraphics->gl.glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // Must use nearest/nearest filtering in order for palettes to work properly.
    if (components == 1) {
        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        pGraphics->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if (pGraphics->pCurrentTexture) {
        pGraphics->gl.glBindTexture(GL_TEXTURE_2D, pGraphics->pCurrentTexture->objectGL);
    }


    taTexture* pTexture = malloc(sizeof(*pTexture));
    if (pTexture == NULL) {
        pGraphics->gl.glDeleteTextures(1, &objectGL);
        return NULL;
    }

    pTexture->pGraphics = pGraphics;
    pTexture->objectGL = objectGL;
    pTexture->width = width;
    pTexture->height = height;
    pTexture->components = components;

    return pTexture;
}

void taDeleteTexture(taTexture* pTexture)
{
    pTexture->pGraphics->gl.glDeleteTextures(1, &pTexture->objectGL);
    free(pTexture);
}


taMesh* taCreateEmptyMesh(taGraphicsContext* pGraphics, taPrimitiveType primitiveType, taVertexFormat vertexFormat, taIndexFormat indexFormat)
{
    if (pGraphics == NULL) {
        return NULL;
    }

    taMesh* pMesh = calloc(1, sizeof(*pMesh));
    if (pMesh == NULL) {
        return NULL;
    }

    pMesh->pGraphics = pGraphics;
    pMesh->vertexFormat = vertexFormat;
    pMesh->indexFormat = indexFormat;

    if (primitiveType == taPrimitiveTypePoint) {
        pMesh->primitiveTypeGL = GL_POINTS;
    } else if (primitiveType == taPrimitiveTypeLine) {
        pMesh->primitiveTypeGL = GL_LINES;
    } else if (primitiveType == taPrimitiveTypeTriangle) {
        pMesh->primitiveTypeGL = GL_TRIANGLES;
    } else {
        pMesh->primitiveTypeGL = GL_QUADS;
    }

    if (indexFormat == taIndexFormatUInt16) {
        pMesh->indexFormatGL = GL_UNSIGNED_SHORT;
    } else {
        pMesh->indexFormatGL = GL_UNSIGNED_INT;
    }

    return pMesh;
}

taMesh* taCreateMesh(taGraphicsContext* pGraphics, taPrimitiveType primitiveType, taVertexFormat vertexFormat, taUInt32 vertexCount, const void* pVertexData, taIndexFormat indexFormat, taUInt32 indexCount, const void* pIndexData)
{
    taMesh* pMesh = taCreateEmptyMesh(pGraphics, primitiveType, vertexFormat, indexFormat);
    if (pMesh == NULL) {
        return NULL;
    }


    taUInt32 vertexBufferSize = vertexCount;
    if (vertexFormat == taVertexFormatP2T2) {
        vertexBufferSize *= sizeof(taVertexP2T2);
    } else if (vertexFormat == taVertexFormatP3T2) {
        vertexBufferSize *= sizeof(taVertexP3T2);
    } else {
        vertexBufferSize *= sizeof(taVertexP3T2N3);
    }

    taUInt32 indexBufferSize = indexCount * ((taUInt32)indexFormat);


    if (pGraphics->supportsVBO)
    {
        // Use VBO's.
        pMesh->pVertexData = NULL;
        pMesh->pIndexData = NULL;

        GLuint buffers[2];  // 0 = VBO; 1 = IBO.
        pGraphics->gl.glGenBuffers(2, buffers);


        // Vertices.
        pMesh->vertexObjectGL = buffers[0];
        pGraphics->gl.glBindBuffer(GL_ARRAY_BUFFER, pMesh->vertexObjectGL);
        pGraphics->gl.glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, pVertexData, GL_STATIC_DRAW);

        // Indices.
        pMesh->indexObjectGL = buffers[1];
        pGraphics->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexObjectGL);
        pGraphics->gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, pIndexData, GL_STATIC_DRAW);
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

taMesh* taCreateMutableMesh(taGraphicsContext* pGraphics, taPrimitiveType primitiveType, taVertexFormat vertexFormat, taUInt32 vertexCount, const void* pVertexData, taIndexFormat indexFormat, taUInt32 indexCount, const void* pIndexData)
{
    taMesh* pMesh = taCreateEmptyMesh(pGraphics, primitiveType, vertexFormat, indexFormat);
    if (pMesh == NULL) {
        return NULL;
    }

    taUInt32 vertexBufferSize = vertexCount;
    if (vertexFormat == taVertexFormatP2T2) {
        vertexBufferSize *= sizeof(taVertexP2T2);
    } else if (vertexFormat == taVertexFormatP3T2) {
        vertexBufferSize *= sizeof(taVertexP3T2);
    } else {
        vertexBufferSize *= sizeof(taVertexP3T2N3);
    }

    pMesh->pVertexData = malloc(vertexBufferSize);
    if (pMesh->pVertexData == NULL) {
        free(pMesh);
        return NULL;
    }

    if (pVertexData) {
        memcpy(pMesh->pVertexData, pVertexData, vertexBufferSize);
    }


    taUInt32 indexBufferSize = indexCount * ((taUInt32)indexFormat);
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

void taDeleteMesh(taMesh* pMesh)
{
    if (pMesh == NULL) {
        return;
    }

    if (pMesh->vertexObjectGL) {
        pMesh->pGraphics->gl.glDeleteBuffers(1, &pMesh->vertexObjectGL);
    }
    if (pMesh->indexObjectGL) {
        pMesh->pGraphics->gl.glDeleteBuffers(1, &pMesh->indexObjectGL);
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

taUInt16 taGetMaxTextureSize(taGraphicsContext* pGraphics)
{
    if (pGraphics == NULL) {
        return 0;
    }

    return (taUInt16)pGraphics->maxTextureSize;
}



//// Drawing ////

void taSetResolution(taGraphicsContext* pGraphics, unsigned int resolutionX, unsigned int resolutionY)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->resolutionX = (GLsizei)resolutionX;
    pGraphics->resolutionY = (GLsizei)resolutionY;
    pGraphics->gl.glViewport(0, 0, pGraphics->resolutionX, pGraphics->resolutionY);
}

void taSetCameraPosition(taGraphicsContext* pGraphics, int posX, int posY)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->cameraPosX = posX;
    pGraphics->cameraPosY = posY;
}

void taTranslateCamera(taGraphicsContext* pGraphics, int offsetX, int offsetY)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->cameraPosX += offsetX;
    pGraphics->cameraPosY += offsetY;

    //printf("Camera Pos: %d %d\n", pGraphics->cameraPosX, pGraphics->cameraPosY);
}

static TA_INLINE void taGraphicsBindTexture(taGraphicsContext* pGraphics, taTexture* pTexture)
{
    assert(pGraphics != NULL);

    if (pGraphics->pCurrentTexture == pTexture) {
        return;
    }

    if (pTexture != NULL) {
        pGraphics->gl.glBindTexture(GL_TEXTURE_2D, pTexture->objectGL);
    } else {
        pGraphics->gl.glBindTexture(GL_TEXTURE_2D, 0);
    }

    pGraphics->pCurrentTexture = pTexture;
}

static TA_INLINE void taGraphicsBindVertexProgram(taGraphicsContext* pGraphics, GLuint vertexProgram)
{
    assert(pGraphics != NULL);

    if (pGraphics->currentVertexProgram == vertexProgram) {
        return;
    }

    if (vertexProgram == 0) {
        pGraphics->gl.glDisable(GL_VERTEX_PROGRAM_ARB);
    } else {
        pGraphics->gl.glEnable(GL_VERTEX_PROGRAM_ARB);
    }

    pGraphics->gl.glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexProgram);
    pGraphics->currentVertexProgram = vertexProgram;
}

static TA_INLINE void taGraphicsBindFragmentProgram(taGraphicsContext* pGraphics, GLuint fragmentProgram)
{
    assert(pGraphics != NULL);

    if (pGraphics->currentFragmentProgram == fragmentProgram) {
        return;
    }

    if (fragmentProgram == 0) {
        pGraphics->gl.glDisable(GL_FRAGMENT_PROGRAM_ARB);
    } else {
        pGraphics->gl.glEnable(GL_FRAGMENT_PROGRAM_ARB);
    }

    pGraphics->gl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, fragmentProgram);
    pGraphics->currentFragmentProgram = fragmentProgram;
}

static TA_INLINE void taGraphicsBindShader(taGraphicsContext* pGraphics, taGraphicsShader* pShader)
{
    assert(pGraphics != NULL);

    if (pShader != NULL) {
        taGraphicsBindVertexProgram(pGraphics, pShader->vertexProgram);
        taGraphicsBindFragmentProgram(pGraphics, pShader->fragmentProgram);
    } else {
        taGraphicsBindVertexProgram(pGraphics, 0);
        taGraphicsBindFragmentProgram(pGraphics, 0);
    }
}



static TA_INLINE void taGraphicsBindMesh(taGraphicsContext* pGraphics, taMesh* pMesh)
{
    assert(pGraphics != NULL);
    
    if (pGraphics->pCurrentMesh == pMesh) {
        return;
    }

    pGraphics->gl.glDisableClientState(GL_NORMAL_ARRAY);

    if (pMesh != NULL) {
        if (pMesh->pVertexData != NULL) {
            // Using vertex arrays.
            if (pGraphics->supportsVBO) {
                pGraphics->gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
                pGraphics->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }

            if (pMesh->vertexFormat == taVertexFormatP2T2) {
                pGraphics->gl.glVertexPointer(2, GL_FLOAT, sizeof(taVertexP2T2), pMesh->pVertexData);
                pGraphics->gl.glTexCoordPointer(2, GL_FLOAT, sizeof(taVertexP2T2), ((taUInt8*)pMesh->pVertexData) + (2*sizeof(float)));
            } else if (pMesh->vertexFormat == taVertexFormatP3T2) {
                pGraphics->gl.glVertexPointer(3, GL_FLOAT, sizeof(taVertexP3T2), pMesh->pVertexData);
                pGraphics->gl.glTexCoordPointer(2, GL_FLOAT, sizeof(taVertexP3T2), ((taUInt8*)pMesh->pVertexData) + (3*sizeof(float)));
            } else {
                pGraphics->gl.glEnableClientState(GL_NORMAL_ARRAY);
                pGraphics->gl.glVertexPointer(3, GL_FLOAT, sizeof(taVertexP3T2N3), pMesh->pVertexData);
                pGraphics->gl.glTexCoordPointer(2, GL_FLOAT, sizeof(taVertexP3T2N3), ((taUInt8*)pMesh->pVertexData) + (3*sizeof(float)));
                pGraphics->gl.glNormalPointer(GL_FLOAT, sizeof(taVertexP3T2N3), ((taUInt8*)pMesh->pVertexData) + (5*sizeof(float)));
            }
        } else if (pMesh->vertexObjectGL) {
            pGraphics->gl.glBindBuffer(GL_ARRAY_BUFFER, pMesh->vertexObjectGL);
            pGraphics->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pMesh->indexObjectGL);

            if (pMesh->vertexFormat == taVertexFormatP2T2) {
                pGraphics->gl.glVertexPointer(2, GL_FLOAT, sizeof(taVertexP2T2), 0);
                pGraphics->gl.glTexCoordPointer(2, GL_FLOAT, sizeof(taVertexP2T2), (const GLvoid*)(2*sizeof(float)));
            } else if (pMesh->vertexFormat == taVertexFormatP3T2) {
                pGraphics->gl.glVertexPointer(3, GL_FLOAT, sizeof(taVertexP3T2), 0);
                pGraphics->gl.glTexCoordPointer(2, GL_FLOAT, sizeof(taVertexP3T2), (const GLvoid*)(3*sizeof(float)));
            } else {
                pGraphics->gl.glEnableClientState(GL_NORMAL_ARRAY);
                pGraphics->gl.glVertexPointer(3, GL_FLOAT, sizeof(taVertexP3T2N3), 0);
                pGraphics->gl.glTexCoordPointer(2, GL_FLOAT, sizeof(taVertexP3T2N3), (const GLvoid*)(3*sizeof(float)));
                pGraphics->gl.glNormalPointer(GL_FLOAT, sizeof(taVertexP3T2N3), (const GLvoid*)(5*sizeof(float)));
            }
        }
    } else {
        pGraphics->gl.glVertexPointer(4, GL_FLOAT, 0, NULL);
        pGraphics->gl.glTexCoordPointer(4, GL_FLOAT, 0, NULL);
        pGraphics->gl.glNormalPointer(GL_FLOAT, 0, NULL);

        if (pGraphics->supportsVBO) {
            pGraphics->gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
            pGraphics->gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
    }

    pGraphics->pCurrentMesh = pMesh;
}

static TA_INLINE void taGraphicsDrawMesh(taGraphicsContext* pGraphics, taMesh* pMesh, taUInt32 indexCount, taUInt32 indexOffset)
{
    // Pre: The mesh is assumed to be bound.
    assert(pGraphics != NULL);
    assert(pGraphics->pCurrentMesh == pMesh);
    assert(pMesh != NULL);

    (void)pGraphics;

    taUInt32 byteOffset = indexOffset * ((taUInt32)pMesh->indexFormat);

    if (pMesh->pIndexData != NULL) {
        pGraphics->gl.glDrawElements(pMesh->primitiveTypeGL, indexCount, pMesh->indexFormatGL, (taUInt8*)pMesh->pIndexData + byteOffset);
    } else {
        pGraphics->gl.glDrawElements(pMesh->primitiveTypeGL, indexCount, pMesh->indexFormatGL, (const GLvoid*)((taUInt8*)0 + byteOffset));
    }
}

#define TA_GUI_CLEAR_MODE_BLACK 0
#define TA_GUI_CLEAR_MODE_SHADE 1

void taDrawGUI(taGraphicsContext* pGraphics, taGUI* pGUI, taUInt32 clearMode)
{
    if (pGraphics == NULL || pGUI == NULL) {
        return;
    }

    // Fullscreen GUIs are drawn based on a 640x480 resolution. We want to stretch the GUI, but maintain it's aspect ratio.
    float scale   = 1;
    float offsetX = 0;
    float offsetY = 0;
    taGUIGetScreenMapping(pGUI, pGraphics->resolutionX, pGraphics->resolutionY, &scale, &offsetX, &offsetY);

    float quadLeft   = 0;
    float quadTop    = 0;
    float quadRight  = (float)pGUI->pGadgets[0].width;
    float quadBottom = (float)pGUI->pGadgets[0].height;

    quadLeft   *= scale;
    quadTop    *= scale;
    quadRight  *= scale;
    quadBottom *= scale;

    quadLeft   += offsetX;
    quadTop    += offsetY;
    quadRight  += offsetX;
    quadBottom += offsetY;

    pGraphics->gl.glMatrixMode(GL_PROJECTION);
    pGraphics->gl.glLoadIdentity();
    pGraphics->gl.glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);

    pGraphics->gl.glMatrixMode(GL_MODELVIEW);
    pGraphics->gl.glLoadIdentity();

    if (clearMode == TA_GUI_CLEAR_MODE_BLACK) {
        pGraphics->gl.glClearDepth(1.0);
        pGraphics->gl.glClearColor(0, 0, 0, 0);
        pGraphics->gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        pGraphics->gl.glEnable(GL_BLEND); // <-- This is disabled below.
        taGraphicsBindShader(pGraphics, NULL);
        taGraphicsBindTexture(pGraphics, NULL);
        pGraphics->gl.glBegin(GL_QUADS);
        {
            pGraphics->gl.glColor4f(0, 0, 0, 0.5f); pGraphics->gl.glVertex3f(0,                             (float)pGraphics->resolutionY, 0.0f);
            pGraphics->gl.glColor4f(0, 0, 0, 0.5f); pGraphics->gl.glVertex3f((float)pGraphics->resolutionX, (float)pGraphics->resolutionY, 0.0f);
            pGraphics->gl.glColor4f(0, 0, 0, 0.5f); pGraphics->gl.glVertex3f((float)pGraphics->resolutionX, 0,                             0.0f);
            pGraphics->gl.glColor4f(0, 0, 0, 0.5f); pGraphics->gl.glVertex3f(0,                             0,                             0.0f);
            pGraphics->gl.glColor4f(1, 1, 1, 1);
        }
        pGraphics->gl.glEnd();
    }

    pGraphics->gl.glDisable(GL_BLEND);

    if (pGUI->pBackgroundTexture != NULL) {
        taGraphicsBindShader(pGraphics, NULL);
        taGraphicsBindTexture(pGraphics, pGUI->pBackgroundTexture);
        pGraphics->gl.glBegin(GL_QUADS);
        {
            float uvleft   = 0;
            float uvtop    = 0;
            float uvright  = (float)pGUI->pGadgets[0].width  / 640.0f;
            float uvbottom = (float)pGUI->pGadgets[0].height / 480.0f;

            pGraphics->gl.glTexCoord2f(uvleft,  uvbottom); pGraphics->gl.glVertex3f(quadLeft,  quadBottom, 0.0f);
            pGraphics->gl.glTexCoord2f(uvright, uvbottom); pGraphics->gl.glVertex3f(quadRight, quadBottom, 0.0f);
            pGraphics->gl.glTexCoord2f(uvright, uvtop);    pGraphics->gl.glVertex3f(quadRight, quadTop,    0.0f);
            pGraphics->gl.glTexCoord2f(uvleft,  uvtop);    pGraphics->gl.glVertex3f(quadLeft,  quadTop,    0.0f);
        }
        pGraphics->gl.glEnd();
    }

    // Gadgets, not including the root.
    for (taUInt32 iGadget = 1; iGadget < pGUI->gadgetCount; ++iGadget) {   // <-- Start at 1 to skip the root gadget.
        taGUIGadget* pGadget = pGUI->pGadgets + iGadget;
        if (pGadget->active == 0) {
            continue;   // Skip over inactive gadgets.
        }

        float posX  = pGadget->xpos   * scale + offsetX;
        float posY  = pGadget->ypos   * scale + offsetY;
        float sizeX = pGadget->width  * scale;
        float sizeY = pGadget->height * scale;
        taBool32 isGadgetPressed = pGadget->isHeld && pGUI->hoveredGadgetIndex == iGadget;

        switch (pGadget->id)
        {
            case TA_GUI_GADGET_TYPE_BUTTON:
            {
                // TODO: This highlight is a bit ugly. I think the original game uses modulation for the effect rather than blending
                // a quad.
                if (pGUI->focusedGadgetIndex == iGadget) {
                    float highlightPosX  =  posX - (6*scale);
                    float highlightPosY  =  posY - (6*scale);
                    float highlightSizeX = sizeX + (6*scale)*2;
                    float highlightSizeY = sizeY + (6*scale)*2;

                    pGraphics->gl.glEnable(GL_BLEND);
                    taGraphicsBindShader(pGraphics, NULL);
                    taGraphicsBindTexture(pGraphics, NULL);
                    pGraphics->gl.glBegin(GL_QUADS);
                    {
                        pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX,                highlightPosY+highlightSizeY, 0.0f);
                        pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX+highlightSizeX, highlightPosY+highlightSizeY, 0.0f);
                        pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX+highlightSizeX, highlightPosY,                0.0f);
                        pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX,                highlightPosY,                0.0f);
                        pGraphics->gl.glColor4f(1, 1, 1, 1);
                    }
                    pGraphics->gl.glEnd();
                    pGraphics->gl.glDisable(GL_BLEND);
                }

                if (pGadget->state.button.pBackgroundTextureGroup != NULL) {
                    taUInt32 buttonState = (isGadgetPressed) ? TA_GUI_BUTTON_STATE_PRESSED : TA_GUI_BUTTON_STATE_NORMAL;
                    if (pGadget->state.button.grayedout) {
                        buttonState = TA_GUI_BUTTON_STATE_DISABLED;
                    }

                    taGAFTextureGroupFrame* pFrame = NULL;
                    if (pGadget->state.button.stages == 0) {
                        pFrame = pGadget->state.button.pBackgroundTextureGroup->pFrames + pGadget->state.button.iBackgroundFrame + buttonState;
                    } else {
                        if (buttonState == TA_GUI_BUTTON_STATE_NORMAL) {
                            pFrame = pGadget->state.button.pBackgroundTextureGroup->pFrames + pGadget->state.button.iBackgroundFrame + pGadget->state.button.currentStage;
                        } else {
                            pFrame = pGadget->state.button.pBackgroundTextureGroup->pFrames + pGadget->state.button.iBackgroundFrame + pGadget->state.button.stages + (buttonState-1);
                        }
                    }

                    taTexture* pBackgroundTexture = pGadget->state.button.pBackgroundTextureGroup->ppAtlases[pFrame->atlasIndex];
                    taDrawSubTexture(pBackgroundTexture, posX, posY, pFrame->sizeX*scale, pFrame->sizeY*scale, TA_FALSE, pFrame->atlasPosX, pFrame->atlasPosY, pFrame->sizeX, pFrame->sizeY);
                }

                const char* text = taGUIGetButtonText(pGadget, pGadget->state.button.currentStage);
                if (!taIsStringNullOrEmpty(text)) {
                    float textSizeX;
                    float textSizeY;
                    taFontMeasureText(&pGraphics->pEngine->font, scale, text, &textSizeX, &textSizeY);

                    float textPosX = posX + (sizeX - textSizeX)/2;
                    float textPosY = posY + (sizeY - textSizeY)/2 - (4*scale);

                    // Left-align text for multi-stage buttons.
                    if (pGadget->state.button.stages > 0) {
                        textPosX = posX + (3*scale);
                    }

                    // Slightly indent the text if the button is pressed.
                    if (isGadgetPressed) {
                        textPosX += 1*scale;
                        textPosY += 1*scale;
                    }

                    taDrawText(pGraphics, &pGraphics->pEngine->font, 255, scale, textPosX, textPosY, text);

                    if (pGadget->state.button.quickkey != 0 && pGadget->state.button.stages == 0) {
                        float charPosX;
                        float charPosY;
                        float charSizeX;
                        float charSizeY;
                        if (taFontFindCharacterMetrics(&pGraphics->pEngine->font, scale, text, (char)pGadget->state.button.quickkey, &charPosX, &charPosY, &charSizeX, &charSizeY) == TA_SUCCESS) {
                            float underlineHeight = roundf(1*scale);
                            float underlineOffsetY = roundf(0*scale);
                            charPosX += textPosX;
                            charPosY += textPosY;

                            taUInt32 underlineRGBA = (isGadgetPressed) ? pGraphics->pEngine->palette[0] : pGraphics->pEngine->palette[2];
                            float underlineR = ((underlineRGBA & 0x00FF0000) >> 16) / 255.0f;
                            float underlineG = ((underlineRGBA & 0x0000FF00) >>  8) / 255.0f;
                            float underlineB = ((underlineRGBA & 0x000000FF) >>  0) / 255.0f;

                            taGraphicsBindShader(pGraphics, NULL);
                            taGraphicsBindTexture(pGraphics, NULL);
                            pGraphics->gl.glBegin(GL_QUADS);
                            {
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX,           charPosY+charSizeY+underlineOffsetY+underlineHeight, 0.0f);
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX+charSizeX, charPosY+charSizeY+underlineOffsetY+underlineHeight, 0.0f);
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX+charSizeX, charPosY+charSizeY+underlineOffsetY,                 0.0f);
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX,           charPosY+charSizeY+underlineOffsetY,                 0.0f);
                                pGraphics->gl.glColor3f(1, 1, 1);
                            }
                            pGraphics->gl.glEnd();
                        }
                    }
                }
            } break;

            case TA_GUI_GADGET_TYPE_LISTBOX:
            {
                const float itemPadding = 0;
                float itemPosX = 0;
                float itemPosY = -4*scale;
                for (taUInt32 iItem = pGadget->state.listbox.scrollPos; iItem < pGadget->state.listbox.scrollPos + pGadget->state.listbox.pageSize && iItem < pGadget->state.listbox.itemCount; ++iItem) {
                    taDrawText(pGraphics, &pGraphics->pEngine->font, 255, scale, posX + itemPosX, posY + itemPosY, pGadget->state.listbox.pItems[iItem]);
                    if (iItem == pGadget->state.listbox.iSelectedItem) {
                        float highlightPosX  = posX + itemPosX - (0*scale);
                        float highlightPosY  = posY + itemPosY + (4*scale);
                        float highlightSizeX = sizeX + (0*scale)*2;
                        float highlightSizeY = pGraphics->pEngine->font.height*scale + (0*scale)*2;

                        pGraphics->gl.glEnable(GL_BLEND);
                        taGraphicsBindShader(pGraphics, NULL);
                        taGraphicsBindTexture(pGraphics, NULL);
                        pGraphics->gl.glBegin(GL_QUADS);
                        {
                            pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX,                highlightPosY+highlightSizeY, 0.0f);
                            pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX+highlightSizeX, highlightPosY+highlightSizeY, 0.0f);
                            pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX+highlightSizeX, highlightPosY,                0.0f);
                            pGraphics->gl.glColor4f(1, 1, 1, 0.15f); pGraphics->gl.glVertex3f(highlightPosX,                highlightPosY,                0.0f);
                            pGraphics->gl.glColor4f(1, 1, 1, 1);
                        }
                        pGraphics->gl.glEnd();
                        pGraphics->gl.glDisable(GL_BLEND);
                    }

                    itemPosY += (pGraphics->pEngine->font.height + (itemPadding*2)) * scale;
                    if (itemPosY >= sizeY) {
                        break;  // Reached the last visible item.
                    }
                }
            } break;

            case TA_GUI_GADGET_TYPE_TEXTBOX:
            {
            } break;

            case TA_GUI_GADGET_TYPE_SCROLLBAR:
            {
                taGAFTextureGroupFrame* pArrow0Frame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iArrow0Frame; // UP/LEFT arrow
                taGAFTextureGroupFrame* pArrow1Frame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iArrow1Frame; // DOWN/RIGHT arrow
                taTexture* pArrow0Texture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pArrow0Frame->atlasIndex];
                taTexture* pArrow1Texture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pArrow1Frame->atlasIndex];
                float arrow0PosX = 0;
                float arrow0PosY = 0;
                float arrow1PosX = 0;
                float arrow1PosY = 0;

                taGAFTextureGroupFrame* pTrackBegFrame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iTrackBegFrame;
                taGAFTextureGroupFrame* pTrackEndFrame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iTrackEndFrame;
                taGAFTextureGroupFrame* pTrackMidFrame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iTrackMidFrame;
                taTexture* pTrackBegTexture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pTrackBegFrame->atlasIndex];
                taTexture* pTrackEndTexture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pTrackEndFrame->atlasIndex];
                taTexture* pTrackMidTexture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pTrackMidFrame->atlasIndex]; (void)pTrackMidTexture; /* <-- TODO: Do something with this graphic. */
                float trackBegPosX = 0;
                float trackBegPosY = 0;
                float trackEndPosX = 0;
                float trackEndPosY = 0;

                taGAFTextureGroupFrame* pThumbFrame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iThumbFrame;
                taGAFTextureGroupFrame* pThumbCapTopFrame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iThumbCapTopFrame;
                taGAFTextureGroupFrame* pThumbCapBotFrame = pGadget->state.scrollbar.pTextureGroup->pFrames + pGadget->state.scrollbar.iThumbCapBotFrame;
                taTexture* pThumbTexture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pThumbFrame->atlasIndex];
                taTexture* pThumbCapTopTexture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pThumbCapTopFrame->atlasIndex];
                taTexture* pThumbCapBotTexture = pGadget->state.scrollbar.pTextureGroup->ppAtlases[pThumbCapBotFrame->atlasIndex];
                float thumbBegPosX = 0;
                float thumbBegPosY = 0;
                float thumbEndPosX = 0;
                float thumbEndPosY = 0;

                if ((pGadget->attribs & TA_GUI_SCROLLBAR_TYPE_VERTICAL) != 0) {
                    // Vertical
                    arrow0PosX = posX;
                    arrow0PosY = posY;
                    arrow1PosX = posX;
                    arrow1PosY = posY+sizeY - pArrow1Frame->sizeY*scale;
                    trackBegPosX = arrow0PosX;
                    trackBegPosY = arrow0PosY + pArrow0Frame->sizeY*scale;
                    trackEndPosX = arrow1PosX;
                    trackEndPosY = arrow1PosY - pTrackEndFrame->sizeY*scale;
                    thumbBegPosX = trackBegPosX + (3*scale);
                    thumbBegPosY = trackBegPosY + (3*scale) + pGadget->state.scrollbar.knobpos*scale;
                    thumbEndPosX = thumbBegPosX;
                    thumbEndPosY = thumbBegPosY + pGadget->state.scrollbar.knobsize*scale;
                } else {
                    // Horizontal
                    arrow0PosX = posX;
                    arrow0PosY = posY;
                    arrow1PosX = posX+sizeX - pArrow1Frame->sizeX*scale;
                    arrow1PosY = posY;
                    trackBegPosX = arrow0PosX + pArrow0Frame->sizeX*scale;
                    trackBegPosY = arrow0PosY;
                    trackEndPosX = arrow1PosX - pTrackEndFrame->sizeX*scale;
                    trackEndPosY = arrow1PosY;
                    thumbBegPosX = trackBegPosX + (3*scale) + pGadget->state.scrollbar.knobpos*scale;
                    thumbBegPosY = trackBegPosY + (3*scale);
                    thumbEndPosX = thumbBegPosX + pGadget->state.scrollbar.knobsize*scale;
                    thumbEndPosY = thumbBegPosY;
                }

                // Arrows.
                taDrawSubTexture(pArrow0Texture, arrow0PosX, arrow0PosY, pArrow0Frame->sizeX*scale, pArrow0Frame->sizeY*scale, TA_TRUE, pArrow0Frame->atlasPosX, pArrow0Frame->atlasPosY, pArrow0Frame->sizeX, pArrow0Frame->sizeY);
                taDrawSubTexture(pArrow1Texture, arrow1PosX, arrow1PosY, pArrow1Frame->sizeX*scale, pArrow1Frame->sizeY*scale, TA_TRUE, pArrow1Frame->atlasPosX, pArrow1Frame->atlasPosY, pArrow1Frame->sizeX, pArrow1Frame->sizeY);

                // Track.
                if ((pGadget->attribs & TA_GUI_SCROLLBAR_TYPE_VERTICAL) != 0) {
                    float runningPosY = trackBegPosY + pTrackBegFrame->sizeY*scale;
                    for (;;) {
                        if (runningPosY >= trackEndPosY) {
                            break;
                        }

                        taDrawSubTexture(pTrackBegTexture, trackBegPosX, runningPosY, pTrackMidFrame->sizeX*scale, pTrackMidFrame->sizeY*scale, TA_TRUE, pTrackMidFrame->atlasPosX, pTrackMidFrame->atlasPosY, pTrackMidFrame->sizeX, pTrackMidFrame->sizeY);
                        runningPosY += pTrackMidFrame->sizeY*scale;
                    }
                } else {
                    float runningPosX = trackBegPosY + pTrackBegFrame->sizeY*scale;
                    for (;;) {
                        if (runningPosX >= trackEndPosY) {
                            break;
                        }

                        taDrawSubTexture(pTrackBegTexture, runningPosX, trackBegPosY, pTrackMidFrame->sizeX*scale, pTrackMidFrame->sizeY*scale, TA_TRUE, pTrackMidFrame->atlasPosX, pTrackMidFrame->atlasPosY, pTrackMidFrame->sizeX, pTrackMidFrame->sizeY);
                        runningPosX += pTrackMidFrame->sizeX*scale;
                    }
                }

                taDrawSubTexture(pTrackBegTexture, trackBegPosX, trackBegPosY, pTrackBegFrame->sizeX*scale, pTrackBegFrame->sizeY*scale, TA_TRUE, pTrackBegFrame->atlasPosX, pTrackBegFrame->atlasPosY, pTrackBegFrame->sizeX, pTrackBegFrame->sizeY);
                taDrawSubTexture(pTrackEndTexture, trackEndPosX, trackEndPosY, pTrackEndFrame->sizeX*scale, pTrackEndFrame->sizeY*scale, TA_TRUE, pTrackEndFrame->atlasPosX, pTrackEndFrame->atlasPosY, pTrackEndFrame->sizeX, pTrackEndFrame->sizeY);

                // Thumb.
                if ((pGadget->attribs & TA_GUI_SCROLLBAR_TYPE_VERTICAL) != 0) {
                    float runningPosY = thumbBegPosY;
                    for (;;) {
                        if (runningPosY >= thumbEndPosY) {
                            break;
                        }

                        taDrawSubTexture(pThumbTexture, thumbBegPosX, runningPosY, pThumbFrame->sizeX*scale, pThumbFrame->sizeY*scale, TA_TRUE, pThumbFrame->atlasPosX, pThumbFrame->atlasPosY, pThumbFrame->sizeX, pThumbFrame->sizeY);
                        runningPosY += pThumbFrame->sizeY*scale;
                    }

                    // Caps.
                    taDrawSubTexture(pThumbCapTopTexture, thumbBegPosX, thumbBegPosY,                                 pThumbCapTopFrame->sizeX*scale, pThumbCapTopFrame->sizeY*scale, TA_TRUE, pThumbCapTopFrame->atlasPosX, pThumbCapTopFrame->atlasPosY, pThumbCapTopFrame->sizeX, pThumbCapTopFrame->sizeY);
                    taDrawSubTexture(pThumbCapBotTexture, thumbBegPosX, runningPosY - pThumbCapBotFrame->sizeY*scale, pThumbCapBotFrame->sizeX*scale, pThumbCapBotFrame->sizeY*scale, TA_TRUE, pThumbCapBotFrame->atlasPosX, pThumbCapBotFrame->atlasPosY, pThumbCapBotFrame->sizeX, pThumbCapBotFrame->sizeY);
                } else {
                    // Horizontal scrollbars are a bit different to vertical in that they appear to always be a fixed size (10x10) and use
                    // a different graphic.
                    taDrawSubTexture(pThumbTexture, thumbBegPosX, thumbBegPosY, pThumbFrame->sizeX*scale, pThumbFrame->sizeY*scale, TA_TRUE, pThumbFrame->atlasPosX, pThumbFrame->atlasPosY, pThumbFrame->sizeX, pThumbFrame->sizeY);
                }
            } break;

            case TA_GUI_GADGET_TYPE_LABEL:
            {
                if (!taIsStringNullOrEmpty(pGadget->state.label.text)) {
                    float textSizeX;
                    float textSizeY;
                    taFontMeasureText(&pGraphics->pEngine->fontSmall, scale, pGadget->state.label.text, &textSizeX, &textSizeY);

                    float textPosX = posX + (1*scale);
                    float textPosY = posY - (4*scale);
                    taDrawText(pGraphics, &pGraphics->pEngine->fontSmall, 255, scale, textPosX, textPosY, pGadget->state.label.text);

                    // Underline the shortcut key for the associated button.
                    if (pGadget->state.label.iLinkedGadget != (taUInt32)-1) {
                        taGUIGadget* pLinkedGadget = &pGUI->pGadgets[pGadget->state.label.iLinkedGadget];
                        float charPosX;
                        float charPosY;
                        float charSizeX;
                        float charSizeY;
                        if (taFontFindCharacterMetrics(&pGraphics->pEngine->fontSmall, scale, pGadget->state.label.text, (char)pLinkedGadget->state.button.quickkey, &charPosX, &charPosY, &charSizeX, &charSizeY) == TA_SUCCESS) {
                            float underlineHeight = roundf(1*scale);
                            float underlineOffsetY = roundf(0*scale);
                            charPosX += textPosX;
                            charPosY += textPosY;

                            taUInt32 underlineRGBA = (isGadgetPressed) ? pGraphics->pEngine->palette[0] : pGraphics->pEngine->palette[2];
                            float underlineR = ((underlineRGBA & 0x00FF0000) >> 16) / 255.0f;
                            float underlineG = ((underlineRGBA & 0x0000FF00) >>  8) / 255.0f;
                            float underlineB = ((underlineRGBA & 0x000000FF) >>  0) / 255.0f;

                            taGraphicsBindShader(pGraphics, NULL);
                            taGraphicsBindTexture(pGraphics, NULL);
                            pGraphics->gl.glBegin(GL_QUADS);
                            {
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX,           charPosY+charSizeY+underlineOffsetY+underlineHeight, 0.0f);
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX+charSizeX, charPosY+charSizeY+underlineOffsetY+underlineHeight, 0.0f);
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX+charSizeX, charPosY+charSizeY+underlineOffsetY,                 0.0f);
                                pGraphics->gl.glColor3f(underlineR, underlineG, underlineB); pGraphics->gl.glVertex3f(charPosX,           charPosY+charSizeY+underlineOffsetY,                 0.0f);
                                pGraphics->gl.glColor3f(1, 1, 1);
                            }
                            pGraphics->gl.glEnd();
                        }
                    }
                }
            } break;

            case TA_GUI_GADGET_TYPE_SURFACE:
            {
            } break;

            case TA_GUI_GADGET_TYPE_PICTURE:
            {
            } break;

            default: break;
        }
    }
}

void taDrawFullscreenGUI(taGraphicsContext* pGraphics, taGUI* pGUI)
{
    taDrawGUI(pGraphics, pGUI, TA_GUI_CLEAR_MODE_BLACK);
}

void taDrawDialogGUI(taGraphicsContext* pGraphics, taGUI* pGUI)
{
    taDrawGUI(pGraphics, pGUI, TA_GUI_CLEAR_MODE_SHADE);
}


void taDrawMapTerrain(taGraphicsContext* pGraphics, taMapInstance* pMap)
{
    // Pre: The paletted fragment program should be bound.
    // Pre: Fragment program should be enabled.

    // The terrain is the base layer so there's no need to clear the color buffer - we just draw over it anyway.
    GLbitfield clearFlags = GL_DEPTH_BUFFER_BIT;
    if (pGraphics->cameraPosX < 0 || (taUInt32)pGraphics->cameraPosX + pGraphics->resolutionX > (pMap->terrain.tileCountX * 32) ||
        pGraphics->cameraPosY < 0 || (taUInt32)pGraphics->cameraPosY + pGraphics->resolutionY > (pMap->terrain.tileCountY * 32)) {
        clearFlags |= GL_COLOR_BUFFER_BIT;
    }

    pGraphics->gl.glClear(clearFlags);


    pGraphics->gl.glMatrixMode(GL_PROJECTION);
    pGraphics->gl.glLoadIdentity();
    pGraphics->gl.glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);

    pGraphics->gl.glMatrixMode(GL_MODELVIEW);
    pGraphics->gl.glLoadIdentity();
    pGraphics->gl.glTranslatef((GLfloat)-pGraphics->cameraPosX, (GLfloat)-pGraphics->cameraPosY, 0);


    // The terrain will never be transparent.
    pGraphics->gl.glDisable(GL_BLEND);


    // The mesh must be bound before we can draw it.
    taGraphicsBindMesh(pGraphics, pMap->terrain.pMesh);


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


    for (taInt32 chunkY = 0; chunkY < visibleChunkCountY; ++chunkY) {
        for (taInt32 chunkX = 0; chunkX < visibleChunkCountX; ++chunkX) {
            taMapTerrainChunk* pChunk =  &pMap->terrain.pChunks[((chunkY+firstChunkPosY) * pMap->terrain.chunkCountX) + (chunkX+firstChunkPosX)];
            for (taUInt32 iMesh = 0; iMesh < pChunk->meshCount; ++iMesh) {
                taMapTerrainSubMesh* pSubmesh = &pChunk->pMeshes[iMesh];
                taGraphicsBindTexture(pGraphics, pMap->ppTextures[pSubmesh->textureIndex]);
                taGraphicsDrawMesh(pGraphics, pMap->terrain.pMesh, pSubmesh->indexCount, pSubmesh->indexOffset);
            }
        }
    }
}

void taDrawMapFeatureSequance(taGraphicsContext* pGraphics, taMapInstance* pMap, taMapFeature* pFeature, taMapFeatureSequence* pSequence, taUInt32 frameIndex, taBool32 transparent)
{
    if (pSequence == NULL) {
        return;
    }

    taMapFeatureFrame* pFrame = pSequence->pFrames + frameIndex;

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
        taGraphicsBindShader(pGraphics, &pGraphics->palettedShaderTransparent);
    } else {
        taGraphicsBindShader(pGraphics, &pGraphics->palettedShader);
    }

    taTexture* pTexture = pMap->ppTextures[pSequence->pFrames[pFeature->currentFrameIndex].textureIndex];
    taGraphicsBindTexture(pGraphics, pTexture);


    float uvleft   = (float)pFrame->texturePosX / pTexture->width;
    float uvtop    = (float)pFrame->texturePosY / pTexture->height;
    float uvright  = (float)(pFrame->texturePosX + pFrame->width)  / pTexture->width;
    float uvbottom = (float)(pFrame->texturePosY + pFrame->height) / pTexture->height;

    taVertexP2T2* pVertexData = pGraphics->pFeaturesMesh->pVertexData;
    pVertexData[0].x = posX;                 pVertexData[0].y = posY;                  pVertexData[0].u = uvleft;  pVertexData[0].v = uvtop;
    pVertexData[1].x = posX;                 pVertexData[1].y = posY + pFrame->height; pVertexData[1].u = uvleft;  pVertexData[1].v = uvbottom;
    pVertexData[2].x = posX + pFrame->width; pVertexData[2].y = posY + pFrame->height; pVertexData[2].u = uvright; pVertexData[2].v = uvbottom;
    pVertexData[3].x = posX + pFrame->width; pVertexData[3].y = posY;                  pVertexData[3].u = uvright; pVertexData[3].v = uvtop;
    taGraphicsBindMesh(pGraphics, pGraphics->pFeaturesMesh);
    taGraphicsDrawMesh(pGraphics, pGraphics->pFeaturesMesh, 4, 0);
}

void taDrawMapFeature3DOObjectRecursive(taGraphicsContext* pGraphics, taMapInstance* pMap, taMapFeature* pFeature, taMap3DO* p3DO, taUInt32 objectIndex)
{
    taMap3DOObject* pObject = &p3DO->pObjects[objectIndex];
    assert(pObject != NULL);

    pGraphics->gl.glPushMatrix();
    pGraphics->gl.glTranslatef((float)pObject->relativePosX, (float)pObject->relativePosY, (float)pObject->relativePosZ);
    {
        taGraphicsBindShader(pGraphics, &pGraphics->palettedShader3D);

        for (size_t iMesh = 0; iMesh < pObject->meshCount; ++iMesh) {
            taMap3DOMesh* p3DOMesh = &p3DO->pMeshes[pObject->firstMeshIndex + iMesh];

            taGraphicsBindTexture(pGraphics, pMap->ppTextures[p3DOMesh->textureIndex]);
            taGraphicsBindMesh(pGraphics, p3DOMesh->pMesh);

            taGraphicsDrawMesh(pGraphics, p3DOMesh->pMesh, p3DOMesh->indexCount, p3DOMesh->indexOffset);
        }


        // Children.
        if (pObject->firstChildIndex != 0) {
            taDrawMapFeature3DOObjectRecursive(pGraphics, pMap, pFeature, p3DO, pObject->firstChildIndex);
        }
    }
    pGraphics->gl.glPopMatrix();

    // Siblings.
    if (pObject->nextSiblingIndex) {
        taDrawMapFeature3DOObjectRecursive(pGraphics, pMap, pFeature, p3DO, pObject->nextSiblingIndex);
    }
}

void taDrawMapFeature3DO(taGraphicsContext* pGraphics, taMapInstance* pMap, taMapFeature* pFeature, taMap3DO* p3DO)
{
    assert(pGraphics != NULL);
    assert(pMap != NULL);
    assert(p3DO != NULL);

    float posX = pFeature->posX;
    float posY = pFeature->posY;
    float posZ = pFeature->posZ;

    // Perspective correction for the height.
    posY -= (int)posZ/2;

    pGraphics->gl.glMatrixMode(GL_MODELVIEW);
    pGraphics->gl.glPushMatrix();
    pGraphics->gl.glTranslatef(posX, posY, posZ);
    pGraphics->gl.glScalef(1, 1, 1);
    pGraphics->gl.glRotatef(27.67f, 1, 0, 0);
    {
        // This disable/enable pair can be made more efficient. Consider splitting 2D and 3D objects and render in separate loops.
        pGraphics->gl.glEnable(GL_DEPTH_TEST);
        pGraphics->gl.glDisable(GL_BLEND);
        taDrawMapFeature3DOObjectRecursive(pGraphics, pMap, pFeature, p3DO, 0);
        pGraphics->gl.glEnable(GL_BLEND);
        pGraphics->gl.glDisable(GL_DEPTH_TEST);
    }
    pGraphics->gl.glPopMatrix();
}

void taDrawMap(taGraphicsContext* pGraphics, taMapInstance* pMap)
{
    if (pGraphics == NULL || pMap == NULL) {
        return;
    }

    taGraphicsBindShader(pGraphics, &pGraphics->palettedShader);


    // Terrain is always laid down first.
    taDrawMapTerrain(pGraphics, pMap);


    // Features can be assumed to be transparent.
    pGraphics->gl.glEnable(GL_BLEND);
    pGraphics->gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (taUInt32 iFeature = 0; iFeature < pMap->featureCount; ++iFeature) {
        taMapFeature* pFeature = pMap->pFeatures + iFeature;
        if (pFeature->pType->pSequenceDefault) {
            // Draw the shadow if we have one.
            if (pFeature->pType->pSequenceShadow != NULL && pGraphics->isShadowsEnabled) {
                taDrawMapFeatureSequance(pGraphics, pMap, pFeature, pFeature->pType->pSequenceShadow, pFeature->currentFrameIndex, (pFeature->pType->pDesc->flags & TA_FEATURE_SHADOWTRANSPARENT) != 0);
            }

            if (pFeature->pCurrentSequence != NULL) {
                taDrawMapFeatureSequance(pGraphics, pMap, pFeature, pFeature->pCurrentSequence, pFeature->currentFrameIndex, TA_FALSE);    // "TA_FALSE" means don't use transparency.
            }
        } else {
            // The feature has no default sequence which means it's probably a 3D object.
            if (pFeature->pType->p3DO != NULL) {
                taDrawMapFeature3DO(pGraphics, pMap, pFeature, pFeature->pType->p3DO);
            }
        }
    }
}

void taDrawText(taGraphicsContext* pGraphics, taFont* pFont, taUInt8 colorIndex, float scale, float posX, float posY, const char* text)
{
    if (pGraphics == NULL || pFont == NULL || text == NULL) {
        return;
    }

    pGraphics->gl.glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    pGraphics->gl.glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);

    pGraphics->gl.glMatrixMode(GL_MODELVIEW);
    pGraphics->gl.glLoadIdentity();

    pGraphics->gl.glEnable(GL_BLEND);
    pGraphics->gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (pFont->canBeColored) {
        taGraphicsBindShader(pGraphics, &pGraphics->textShader);
        pGraphics->gl.glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, colorIndex/255.0f, colorIndex/255.0f, colorIndex/255.0f, colorIndex/255.0f);
        pGraphics->gl.glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, TA_TRANSPARENT_COLOR/255.0f, TA_TRANSPARENT_COLOR/255.0f, TA_TRANSPARENT_COLOR/255.0f, TA_TRANSPARENT_COLOR/255.0f);
    } else {
        taGraphicsBindShader(pGraphics, NULL);
    }

    taGraphicsBindTexture(pGraphics, pFont->pTexture);

    // Immediate mode will do for now, but might want to use vertex arrays later.
    float penPosX = posX;
    float penPosY = posY;

    pGraphics->gl.glBegin(GL_QUADS);
    for (;;) {
        unsigned char c = (unsigned char)*text++;
        if (c == '\0') {
            break;
        }

        taFontGlyph glyph = pFont->glyphs[c];
        float glyphSizeX = glyph.sizeX;
        float glyphSizeY = glyph.sizeY;
        float glyphPosX  = penPosX + glyph.originX*scale;
        float glyphPosY  = penPosY + glyph.originY*scale;

        float uvleft   = (float)glyph.u;
        float uvtop    = (float)glyph.v;
        float uvright  = uvleft + (glyphSizeX / pFont->pTexture->width);
        float uvbottom = uvtop  + (glyphSizeY / pFont->pTexture->height);

        pGraphics->gl.glTexCoord2f(uvleft,  uvbottom); pGraphics->gl.glVertex3f(glyphPosX,                    glyphPosY + glyphSizeY*scale, 0.0f);
        pGraphics->gl.glTexCoord2f(uvright, uvbottom); pGraphics->gl.glVertex3f(glyphPosX + glyphSizeX*scale, glyphPosY + glyphSizeY*scale, 0.0f);
        pGraphics->gl.glTexCoord2f(uvright, uvtop);    pGraphics->gl.glVertex3f(glyphPosX + glyphSizeX*scale, glyphPosY,                    0.0f);
        pGraphics->gl.glTexCoord2f(uvleft,  uvtop);    pGraphics->gl.glVertex3f(glyphPosX,                    glyphPosY,                    0.0f);
        
        penPosX += glyphSizeX*scale;
        if (c == '\n') {
            penPosY += pFont->height*scale;
            penPosX  = posX;
        }
    }
    pGraphics->gl.glEnd();
}

void taDrawTextF(taGraphicsContext* pGraphics, taFont* pFont, taUInt8 colorIndex, float scale, float posX, float posY, const char* text, ...)
{
    va_list args;
    va_start(args, text);
    {
        taString formattedText = taMakeStringv(text, args);
        if (formattedText) {
            taDrawText(pGraphics, pFont, colorIndex, scale, posX, posY, formattedText);
            taFreeString(formattedText);
        }
    }
    va_end(args);
}

void taDrawSubTexture(taTexture* pTexture, float posX, float posY, float width, float height, taBool32 transparent, float subtexturePosX, float subtexturePosY, float subtextureSizeX, float subtextureSizeY)
{
    if (pTexture == NULL) {
        return;
    }

    taGraphicsContext* pGraphics = pTexture->pGraphics;

    pGraphics->gl.glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    pGraphics->gl.glOrtho(0, pGraphics->resolutionX, pGraphics->resolutionY, 0, -1000, 1000);
    
    pGraphics->gl.glMatrixMode(GL_MODELVIEW);
    pGraphics->gl.glLoadIdentity();

    if (transparent) {
        pGraphics->gl.glEnable(GL_BLEND);
        pGraphics->gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        pGraphics->gl.glDisable(GL_BLEND);
    }


    // We need to use a different fragment program depending on whether or not we're using a paletted texture.
    taBool32 isPaletted = pTexture->components == 1;
    if (isPaletted) {
        taGraphicsBindShader(pGraphics, &pGraphics->palettedShader);
    } else {
        taGraphicsBindShader(pGraphics, NULL);
    }
    

    taGraphicsBindTexture(pGraphics, pTexture);
    pGraphics->gl.glBegin(GL_QUADS);
    {
        float uvleft   = subtexturePosX / pTexture->width;
        float uvtop    = subtexturePosY / pTexture->height;
        float uvright  = (subtexturePosX + subtextureSizeX) / pTexture->width;
        float uvbottom = (subtexturePosY + subtextureSizeY) / pTexture->height;

        pGraphics->gl.glTexCoord2f(uvleft,  uvbottom); pGraphics->gl.glVertex3f(posX,         posY + height, 1.0f);
        pGraphics->gl.glTexCoord2f(uvright, uvbottom); pGraphics->gl.glVertex3f(posX + width, posY + height, 1.0f);
        pGraphics->gl.glTexCoord2f(uvright, uvtop);    pGraphics->gl.glVertex3f(posX + width, posY,          1.0f);
        pGraphics->gl.glTexCoord2f(uvleft,  uvtop);    pGraphics->gl.glVertex3f(posX,         posY,          1.0f);
    }
    pGraphics->gl.glEnd();


    // Restore default state.
    if (transparent) {
        pGraphics->gl.glDisable(GL_BLEND);
    }
}


//// Settings ////

void taGraphicsSetEnableShadows(taGraphicsContext* pGraphics, taBool32 isShadowsEnabled)
{
    if (pGraphics == NULL) {
        return;
    }

    pGraphics->isShadowsEnabled = isShadowsEnabled;
}

taBool32 taGraphicsGetEnableShadows(taGraphicsContext* pGraphics)
{
    if (pGraphics == NULL) {
        return TA_FALSE;
    }

    return pGraphics->isShadowsEnabled;
}



// TESTING
void taDrawTexture(taTexture* pTexture, taBool32 transparent,float scale)
{
    taDrawSubTexture(pTexture, 0, 0, (float)pTexture->width*scale, (float)pTexture->height*scale, transparent, 0, 0, (float)pTexture->width, (float)pTexture->height);
}