// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct taTexture taTexture;
typedef struct taMesh taMesh;

typedef struct
{
    float x;
    float y;
    float u;
    float v;
} taVertexP2T2;

typedef struct
{
    float x;
    float y;
    float z;
    float u;
    float v;
} taVertexP3T2;

typedef struct
{
    float x;
    float y;
    float z;
    float u;
    float v;
    float nx;
    float ny;
    float nz;
} taVertexP3T2N3;

typedef struct
{
    float texturePosX;  // The position of the subtexture within the main texture atlas.
    float texturePosY;
    float width;
    float height;
} taSubTextureMetrics;

typedef enum
{
    taVertexFormatUnknown = 0,
    taVertexFormatP2T2,
    taVertexFormatP3T2,
    taVertexFormatP3T2N3
} taVertexFormat;

typedef enum
{
    taIndexFormatUInt16 = 2,
    taIndexFormatUInt32 = 4
} taIndexFormat;  // Always make sure these ones are equal to the size in bytes of the respective type.

typedef enum
{
    taPrimitiveTypePoint,
    taPrimitiveTypeLine,
    taPrimitiveTypeTriangle,
    taPrimitiveTypeQuad
} taPrimitiveType;

typedef enum
{
    taColorModePalette,
    taColorModeTrueColor
} taColorMode;


// Creates a new graphics.
taGraphicsContext* taCreateGraphicsContext(taEngineContext* pEngine, taUInt32 palette[256]);

// Deletes the given graphics context.
void taDeleteGraphicsContext(taGraphicsContext* pGraphics);


// Creates a window to render to.
taWindow* taGraphicsCreateWindow(taGraphicsContext* pGraphics, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options);

// Deletes the given window.
void taGraphicsDeleteWindow(taGraphicsContext* pGraphics, taWindow* pWindow);

// Sets the window to render to.
void taGraphicsSetCurrentWindow(taGraphicsContext* pGraphics, taWindow* pWindow);


// Enables v-sync for the given window.
void taGraphicsEnableVSync(taGraphicsContext* pGraphics, taWindow* pWindow);

// Disables v-sync for the given window.
void taGraphicsDisableVSync(taGraphicsContext* pGraphics, taWindow* pWindow);

// Presents the back buffer of the given window.
void taGraphicsPresent(taGraphicsContext* pGraphics, taWindow* pWindow);


// Creates a texture.
taTexture* taCreateTexture(taGraphicsContext* pGraphics, unsigned int width, unsigned int height, unsigned int components, const void* pImageData);

// Deletes the given texture.
void taDeleteTexture(taTexture* pTexture);


// Creates an immutable mesh.
taMesh* taCreateMesh(taGraphicsContext* pGraphics, taPrimitiveType primitiveType, taVertexFormat vertexFormat, taUInt32 vertexCount, const void* pVertexData, taIndexFormat indexFormat, taUInt32 indexCount, const void* pIndexData);

// Creates a mutable mesh.
taMesh* taCreateMutableMesh(taGraphicsContext* pGraphics, taPrimitiveType primitiveType, taVertexFormat vertexFormat, taUInt32 vertexCount, const void* pVertexData, taIndexFormat indexFormat, taUInt32 indexCount, const void* pIndexData);

// Deletes a mesh.
void taDeleteMesh(taMesh* pMesh);



//// Limits ////

// Retrievs the maximum dimensions of a texture.
unsigned int taGetMaxTextureSize(taGraphicsContext* pGraphics);



//// Drawing ////

// Sets the resolution of the game. This should be called whenever the window changes size.
void taSetResolution(taGraphicsContext* pGraphics, unsigned int resolutionX, unsigned int resolutionY);

// Sets the position of the camera.
void taSetCameraPosition(taGraphicsContext* pGraphics, int posX, int posY);

// Translates the camera.
void taTranslateCamera(taGraphicsContext* pGraphics, int offsetX, int offsetY);


// Draws a full-screen GUI. These kinds of GUIs are things like the main menu. A different function is used for
// doing the in-game GUIs like the HUD.
void taDrawFullscreenGUI(taGraphicsContext* pGraphics, taGUI* pGUI);

// Draws a dialog GUI. This will not usually be fullscreen and will draw a semi-transparent shade over the background
// rather than clearing it.
void taDrawDialogGUI(taGraphicsContext* pGraphics, taGUI* pGUI);

// Draws the given given map.
void taDrawMap(taGraphicsContext* pGraphics, taMapInstance* pMap);

// Draws a string of text.
void taDrawText(taGraphicsContext* pGraphics, taFont* pFont, taUInt8 colorIndex, float scale, float posX, float posY, const char* text);
void taDrawTextf(taGraphicsContext* pGraphics, taFont* pFont, taUInt8 colorIndex, float scale, float posX, float posY, const char* text, ...);

// Draws a textured rectangle.
void taDrawSubTexture(taTexture* pTexture, float posX, float posY, float width, float height, taBool32 transparent, float subtexturePosX, float subtexturePosY, float subtextureSizeX, float subtextureSizeY);


//// Settings ////

// Sets whether or not shadows are enabled.
void taGraphicsSetEnableShadows(taGraphicsContext* pGraphics, taBool32 isShadowsEnabled);

// Determines whether or not shadows are enabled.
taBool32 taGraphicsGetEnableShadows(taGraphicsContext* pGraphics);



// TESTING
void taDrawTexture(taTexture* pTexture, taBool32 transparent, float scale);
