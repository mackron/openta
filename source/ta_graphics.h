// Public domain. See "unlicense" statement at the end of this file.

// Creates a new graphics.
ta_graphics_context* ta_create_graphics_context(ta_game* pGame, uint32_t palette[256]);

// Deletes the given graphics context.
void ta_delete_graphics_context(ta_graphics_context* pGraphics);


// Creates a window to render to.
ta_window* ta_graphics_create_window(ta_graphics_context* pGraphics, const char* pTitle, unsigned int resolutionX, unsigned int resolutionY, unsigned int options);

// Deletes the given window.
void ta_graphics_delete_window(ta_graphics_context* pGraphics, ta_window* pWindow);

// Sets the window to render to.
void ta_graphics_set_current_window(ta_graphics_context* pGraphics, ta_window* pWindow);


// Enables v-sync for the given window.
void ta_graphics_enable_vsync(ta_graphics_context* pGraphics, ta_window* pWindow);

// Disables v-sync for the given window.
void ta_graphics_disable_vsync(ta_graphics_context* pGraphics, ta_window* pWindow);

// Presents the back buffer of the given window.
void ta_graphics_present(ta_graphics_context* pGraphics, ta_window* pWindow);


// Creates a texture.
ta_texture* ta_create_texture(ta_graphics_context* pGraphics, unsigned int width, unsigned int height, unsigned int components, const void* pImageData);

// Deletes the given texture.
void ta_delete_texture(ta_texture* pTexture);


// Retrievs the maximum dimensions of a texture.
unsigned int ta_get_max_texture_size(ta_graphics_context* pGraphics);


//// Drawing ////

// Sets the resolution of the game. This should be called whenever the window changes size.
void ta_set_resolution(ta_graphics_context* pGraphics, unsigned int resolutionX, unsigned int resolutionY);

// Sets the position of the camera.
void ta_set_camera_position(ta_graphics_context* pGraphics, int posX, int posY);

// Translates the camera.
void ta_translate_camera(ta_graphics_context* pGraphics, int offsetX, int offsetY);

// Draws the terrain as defined by the given TNT file.
//
// This will be the first thing to be drawn when drawing the game world. The terrain will be offset based on the camera.
void ta_draw_terrain(ta_graphics_context* pGraphics, ta_tnt* pTNT);



// TESTING
void ta_draw_texture(ta_texture* pTexture, bool transparent);
void ta_draw_subtexture(ta_texture* pTexture, bool transparent, int offsetX, int offsetY, int width, int height);
void ta_draw_tnt_mesh(ta_texture* pTexture, ta_tnt_mesh* pMesh);