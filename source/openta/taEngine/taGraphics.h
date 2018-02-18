// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct ta_texture ta_texture;
typedef struct ta_mesh ta_mesh;

typedef struct
{
    float x;
    float y;
    float u;
    float v;
} ta_vertex_p2t2;

typedef struct
{
    float x;
    float y;
    float z;
    float u;
    float v;
} ta_vertex_p3t2;

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
} ta_vertex_p3t2n3;

typedef struct
{
    float texturePosX;  // The position of the subtexture within the main texture atlas.
    float texturePosY;
    float width;
    float height;
} ta_subtexture_metrics;

typedef enum
{
    ta_vertex_format_unknown = 0,
    ta_vertex_format_p2t2,
    ta_vertex_format_p3t2,
    ta_vertex_format_p3t2n3
} ta_vertex_format;

typedef enum
{
    ta_index_format_uint16 = 2,
    ta_index_format_uint32 = 4
} ta_index_format;  // Always make sure these ones are equal to the size in bytes of the respective type.

typedef enum
{
    ta_primitive_type_point,
    ta_primitive_type_line,
    ta_primitive_type_triangle,
    ta_primitive_type_quad
} ta_primitive_type;

typedef enum
{
    ta_color_mode_palette,
    ta_color_mode_truecolor
} ta_color_mode;


// Creates a new graphics.
ta_graphics_context* ta_create_graphics_context(taEngineContext* pEngine, taUInt32 palette[256]);

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


// Creates an immutable mesh.
ta_mesh* ta_create_mesh(ta_graphics_context* pGraphics, ta_primitive_type primitiveType, ta_vertex_format vertexFormat, taUInt32 vertexCount, const void* pVertexData, ta_index_format indexFormat, taUInt32 indexCount, const void* pIndexData);

// Creates a mutable mesh.
ta_mesh* ta_create_mutable_mesh(ta_graphics_context* pGraphics, ta_primitive_type primitiveType, ta_vertex_format vertexFormat, taUInt32 vertexCount, const void* pVertexData, ta_index_format indexFormat, taUInt32 indexCount, const void* pIndexData);

// Deletes a mesh.
void ta_delete_mesh(ta_mesh* pMesh);



//// Limits ////

// Retrievs the maximum dimensions of a texture.
unsigned int ta_get_max_texture_size(ta_graphics_context* pGraphics);



//// Drawing ////

// Sets the resolution of the game. This should be called whenever the window changes size.
void ta_set_resolution(ta_graphics_context* pGraphics, unsigned int resolutionX, unsigned int resolutionY);

// Sets the position of the camera.
void ta_set_camera_position(ta_graphics_context* pGraphics, int posX, int posY);

// Translates the camera.
void ta_translate_camera(ta_graphics_context* pGraphics, int offsetX, int offsetY);


// Draws a full-screen GUI. These kinds of GUIs are things like the main menu. A different function is used for
// doing the in-game GUIs like the HUD.
void ta_draw_fullscreen_gui(ta_graphics_context* pGraphics, ta_gui* pGUI);

// Draws a dialog GUI. This will not usually be fullscreen and will draw a semi-transparent shade over the background
// rather than clearing it.
void ta_draw_dialog_gui(ta_graphics_context* pGraphics, ta_gui* pGUI);

// Draws the given given map.
void ta_draw_map(ta_graphics_context* pGraphics, ta_map_instance* pMap);

// Draws a string of text.
void ta_draw_text(ta_graphics_context* pGraphics, ta_font* pFont, taUInt8 colorIndex, float scale, float posX, float posY, const char* text);
void ta_draw_textf(ta_graphics_context* pGraphics, ta_font* pFont, taUInt8 colorIndex, float scale, float posX, float posY, const char* text, ...);

// Draws a textured rectangle.
void ta_draw_subtexture(ta_texture* pTexture, float posX, float posY, float width, float height, taBool32 transparent, float subtexturePosX, float subtexturePosY, float subtextureSizeX, float subtextureSizeY);


//// Settings ////

// Sets whether or not shadows are enabled.
void ta_graphics_set_enable_shadows(ta_graphics_context* pGraphics, taBool32 isShadowsEnabled);

// Determines whether or not shadows are enabled.
taBool32 ta_graphics_get_enable_shadows(ta_graphics_context* pGraphics);



// TESTING
void ta_draw_texture(ta_texture* pTexture, taBool32 transparent, float scale);
