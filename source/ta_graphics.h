// Public domain. See "unlicense" statement at the end of this file.

// Creates a new graphics.
ta_graphics_context* ta_create_graphics_context(ta_game* pGame);

// Deletes the given graphics context.
void ta_delete_graphics_context(ta_graphics_context* pGraphics);


// Creates a window to render to.
ta_window* ta_graphics_create_window(ta_graphics_context* pGraphics, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options);

// Deletes the given window.
void ta_graphics_delete_window(ta_graphics_context* pGraphics, ta_window* pWindow);

// Sets the window to render to.
void ta_graphics_set_current_window(ta_graphics_context* pGraphics, ta_window* pWindow);


// Enables v-sync.
void ta_graphics_enable_vsync(ta_graphics_context* pGraphics);

// Disables v-sync.
void ta_graphics_disable_vsync(ta_graphics_context* pGraphics);

// Presents the back buffer of the given window.
void ta_graphics_present(ta_graphics_context* pGraphics, ta_window* pWindow);


// Creates a texture.
ta_texture* ta_create_texture(ta_graphics_context* pGraphics, unsigned int width, unsigned int height, unsigned int components, const void* pImageData);

// Deletes the given texture.
void ta_delete_texture(ta_texture* pTexture);



// TESTING
void ta_draw_texture(ta_texture* pTexture);
