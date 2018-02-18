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
    ta_fs* pFS;
    ta_graphics_context* pGraphics;
    taUInt32 palette[256];     // The standard palette. PALETTE.PAL
    taUInt32 guipal[256];      // The GUI palette. GUIPAL.PAL
    ta_input_state input;
    ta_font font;               // The main font to use for basically all GUI text.
    ta_font fontSmall;
    ta_common_gui commonGUI;
    ta_audio_context* pAudio;

    // The features library. This is initialized once at startup from every TDF file in the "features" directory, and it's sub-directories. The
    // features library is immutable once it's initialized.
    ta_features_library* pFeatures;

    taUInt32 textureGAFCount;
    ta_gaf** ppTextureGAFs;     // The list of GAF files containing textures. This is initialized when the engine context is created.
};

ta_result taEngineContextInit(int argc, char** argv, taLoadPropertiesProc onLoadProperties, taStepProc onStep, void* pUserData, taEngineContext* pEngine);
ta_result taEngineContextUninit(taEngineContext* pEngine);


// Creates a texture from a file.
ta_texture* ta_load_image(taEngineContext* pEngine, const char* filePath);


// Captures the mouse so that all mouse events get directed to the game window.
void ta_capture_mouse(ta_window* pWindow);

// Releases the mouse. The opposite of ta_capture_mouse().
void ta_release_mouse(taEngineContext* pEngine);



//// Events from Window ////

// Called from the window system when the game window is resized.
void ta_on_window_size(taEngineContext* pEngine, unsigned int newWidth, unsigned int newHeight);

// Called when a mouse button is pressed.
void ta_on_mouse_button_down(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags);

// Called when a mouse button is released.
void ta_on_mouse_button_up(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags);

// Called when a mouse button is double clicked.
void ta_on_mouse_button_dblclick(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags);

// Called when the mouse wheel is turned.
void ta_on_mouse_wheel(taEngineContext* pEngine, int delta, int posX, int posY, unsigned int stateFlags);

// Called when the mouse moves.
void ta_on_mouse_move(taEngineContext* pEngine, int posX, int posY, unsigned int stateFlags);

// Called when the mouse enters the window.
void ta_on_mouse_enter(taEngineContext* pEngine);

// Called when the mouse leaves the window.
void ta_on_mouse_leave(taEngineContext* pEngine);

// Called when a key is pressed.
void ta_on_key_down(taEngineContext* pEngine, ta_key key, unsigned int stateFlags);

// Called when a key is released.
void ta_on_key_up(taEngineContext* pEngine, ta_key key, unsigned int stateFlags);

// Called when a printable key is pressed or auto-repeated. Use this for text boxes.
void ta_on_printable_key_down(taEngineContext* pEngine, taUInt32 utf32, unsigned int stateFlags);