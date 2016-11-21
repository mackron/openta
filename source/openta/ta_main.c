// Copyright (C) 2016 David Reid. See included LICENSE file.

// The main file containing the entry point. This is the only compiled file for the entire game.

// Standard headers.
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

// Platform headers. Never expose these publicly. Ever.
#ifdef _WIN32
#include <windows.h>
#endif

// Platform libraries, for simplifying MSVC builds.
#ifdef _WIN32
#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "msimg32.lib")
#endif
#endif


// dr_libs
#define DR_IMPLEMENTATION
#define DR_AUDIO_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#define DR_MATH_IMPLEMENTATION
#define DR_PCX_IMPLEMENTATION
#ifdef TA_USE_EXTERNAL_DR_LIBS
#include "../../../dr_libs/dr.h"
#include "../../../dr_libs/dr_audio.h"
#include "../../../dr_libs/dr_wav.h"
#include "../../../dr_libs/dr_math.h"
#include "../../../dr_libs/dr_pcx.h"
#else
#include "../external/dr_libs/dr.h"
#include "../external/dr_libs/dr_audio.h"
#include "../external/dr_libs/dr_wav.h"
#include "../external/dr_libs/dr_math.h"
#include "../external/dr_libs/dr_pcx.h"
#endif



// Options.

// The color within the palette to use as the transparent pixel.
#define TA_TRANSPARENT_COLOR 240

// The maximum length of a path. This should rarely need to be increased, but try increasing it if you encounter truncation issues.
#define TA_MAX_PATH 256

// The maximum length of a feature name.
#define TA_MAX_FEATURE_NAME 64


// Basic types.
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef   signed char    ta_int8;
typedef unsigned char    ta_uint8;
typedef   signed short   ta_int16;
typedef unsigned short   ta_uint16;
typedef   signed int     ta_int32;
typedef unsigned int     ta_uint32;
typedef   signed __int64 ta_int64;
typedef unsigned __int64 ta_uint64;
#else
#include <stdint.h>
typedef int8_t           ta_int8;
typedef uint8_t          ta_uint8;
typedef int16_t          ta_int16;
typedef uint16_t         ta_uint16;
typedef int32_t          ta_int32;
typedef uint32_t         ta_uint32;
typedef int64_t          ta_int64;
typedef uint64_t         ta_uint64;
#endif
typedef ta_int8          ta_bool8;
typedef ta_int32         ta_bool32;
#define TA_TRUE          1
#define TA_FALSE         0


#define ta_zero_object(p) memset((p), 0, sizeof(*(p)))
#define ta_countof(p) (sizeof((p)) / sizeof((p)[0]))

// The maximum size of the texture atlas. We don't really want to use the GPU's maximum texture size
// because it can result in excessive wastage - modern GPUs support 16K textures, which is much more
// than we need and it's better to not needlessly waste the player's system resources. Keep this at
// a power of 2.
#define TA_MAX_TEXTURE_ATLAS_SIZE   512 /*4096*/

// Total Annihilation headers.
#include "ta_errors.h"
#include "ta_type_declarations.h"
#include "ta_misc.h"
#include "ta_string.h"
#include "ta_platform_layer.h"
#include "ta_texture_packer.h"
#include "ta_mesh_builder.h"
#include "ta_graphics.h"
#include "ta_font.h"
#include "ta_gui.h"
#include "ta_property_manager.h"
#include "ta_game.h"
#include "ta_fs.h"
#include "ta_gaf.h"
#include "ta_3do.h"
#include "ta_config.h"
#include "ta_features.h"
#include "ta_map.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// miniz
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO
#include "../external/miniz.c"

// Bob Jenkins' lookup3 hash
#include "../external/lookup3.c"


// Private Options

// The number of tiles making up a chunk, on each dimension.
#define TA_TERRAIN_CHUNK_SIZE   16


// Total Annihilation source files.
#include "ta_misc.c"
#include "ta_string.c"
#include "ta_platform_layer.c"
#include "ta_texture_packer.c"
#include "ta_mesh_builder.c"
#include "ta_graphics.c"
#include "ta_font.c"
#include "ta_gui.c"
#include "ta_property_manager.c"
#include "ta_game.c"
#include "ta_fs.c"
#include "ta_gaf.c"
#include "ta_3do.c"
#include "ta_config.c"
#include "ta_features.c"
#include "ta_map.c"

int ta_main(dr_cmdline cmdline)
{
    // The window system needs to be initialized once, before creating the game context.
    ta_init_window_system();

    ta_game* pGame = ta_create_game(cmdline);
    if (pGame == NULL) {
        return TA_ERROR_FAILED_TO_CREATE_GAME_CONTEXT;
    }

    int result = ta_game_run(pGame);


    ta_delete_game(pGame);
    ta_uninit_window_system();

    return result;
}

int main(int argc, char** argv)
{
    dr_cmdline cmdline;
    if (!dr_init_cmdline(&cmdline, argc, argv)) {
        return TA_ERROR_FAILED_TO_PARSE_CMDLINE;
    }

    return ta_main(cmdline);
}