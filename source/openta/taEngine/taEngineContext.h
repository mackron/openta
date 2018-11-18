// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef void (* taLoadPropertiesProc)(taEngineContext* pEngine, ta_property_manager* pProperties);
typedef void (* taStepProc)(taEngineContext* pEngine);

struct taEngineContext
{
    int argc;
    char** argv;
    taLoadPropertiesProc onLoadProperties;
    taStepProc onStep;
    void* pUserData;

    ta_property_manager properties;
    taFS* pFS;
    taGraphicsContext* pGraphics;
    taUInt32 palette[256];     // The standard palette. PALETTE.PAL
    taUInt32 guipal[256];      // The GUI palette. GUIPAL.PAL
    taInputState input;
    taFont font;               // The main font to use for basically all GUI text.
    taFont fontSmall;
    taCommonGUI commonGUI;
    taAudioContext* pAudio;

    // The features library. This is initialized once at startup from every TDF file in the "features" directory, and it's sub-directories. The
    // features library is immutable once it's initialized.
    taFeaturesLibrary* pFeatures;

    taUInt32 textureGAFCount;
    taGAF** ppTextureGAFs;     // The list of GAF files containing textures. This is initialized when the engine context is created.
};

taResult taEngineContextInit(int argc, char** argv, taLoadPropertiesProc onLoadProperties, taStepProc onStep, void* pUserData, taEngineContext* pEngine);
taResult taEngineContextUninit(taEngineContext* pEngine);


// Creates a texture from a file.
taTexture* taLoadImage(taEngineContext* pEngine, const char* filePath);


// Captures the mouse so that all mouse events get directed to the game window.
void taCaptureMouse(ta_window* pWindow);

// Releases the mouse. The opposite of taCaptureMouse().
void taReleaseMouse(taEngineContext* pEngine);



//// Events from Window ////

// Called from the window system when the game window is resized.
void taOnWindowSize(taEngineContext* pEngine, unsigned int newWidth, unsigned int newHeight);

// Called when a mouse button is pressed.
void taOnMouseButtonDown(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags);

// Called when a mouse button is released.
void taOnMouseButtonUp(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags);

// Called when a mouse button is double clicked.
void taOnMouseButtonDblClick(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags);

// Called when the mouse wheel is turned.
void taOnMouseWheel(taEngineContext* pEngine, int delta, int posX, int posY, unsigned int stateFlags);

// Called when the mouse moves.
void taOnMouseMove(taEngineContext* pEngine, int posX, int posY, unsigned int stateFlags);

// Called when the mouse enters the window.
void taOnMouseEnter(taEngineContext* pEngine);

// Called when the mouse leaves the window.
void taOnMouseLeave(taEngineContext* pEngine);

// Called when a key is pressed.
void taOnKeyDown(taEngineContext* pEngine, ta_key key, unsigned int stateFlags);

// Called when a key is released.
void taOnKeyUp(taEngineContext* pEngine, ta_key key, unsigned int stateFlags);

// Called when a printable key is pressed or auto-repeated. Use this for text boxes.
void taOnPrintableKeyDown(taEngineContext* pEngine, taUInt32 utf32, unsigned int stateFlags);
