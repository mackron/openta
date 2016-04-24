// Public domain. See "unlicense" statement at the end of this file.

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

// dr_libs headers.
#define DR_GUI_INCLUDE_WIP
#define DR_WAV_NO_STDIO
#define DR_FS_WIN32_USE_EVENT_MUTEX     // For better deadlock detection, but possibly less efficient (need to profile).

#include "../../dr_libs/dr_util.h"
#include "../../dr_libs/dr_path.h"
#include "../../dr_libs/dr_fs.h"
#include "../../dr_libs/dr_gui.h"
#include "../../dr_libs/dr_2d.h"
#include "../../dr_libs/dr_audio.h"
#include "../../dr_libs/dr_wav.h"
#include "../../dr_libs/dr_math.h"
#include "../../dr_libs/dr_pcx.h"

// dr_libs
#define DR_UTIL_IMPLEMENTATION
#define DR_PATH_IMPLEMENTATION
#define DR_FS_IMPLEMENTATION
#define DR_GUI_IMPLEMENTATION
#define DR_2D_IMPLEMENTATION
#define DR_AUDIO_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#define DR_MATH_IMPLEMENTATION
#define DR_PCX_IMPLEMENTATION

#include "../../dr_libs/dr_util.h"
#include "../../dr_libs/dr_path.h"
#include "../../dr_libs/dr_fs.h"
#include "../../dr_libs/dr_gui.h"
#include "../../dr_libs/dr_2d.h"
#include "../../dr_libs/dr_audio.h"
#include "../../dr_libs/dr_wav.h"
#include "../../dr_libs/dr_math.h"
#include "../../dr_libs/dr_pcx.h"


// Options.

// The color within the palette to use as the transparent pixel.
#define TA_TRANSPARENT_COLOR 240

// The maximum length of a path. This should rarely need to be increased, but try it if you encounter truncation issues.
#define TA_MAX_PATH 128


// Total Annihilation headers.
#include "ta_errors.h"
#include "ta_type_declarations.h"
#include "ta_misc.h"
#include "ta_platform_layer.h"
#include "ta_graphics.h"
#include "ta_game.h"
#include "ta_fs.h"
#include "ta_hpi.h"
#include "ta_gaf.h"
#include "ta_tnt.h"
#include "ta_config.h"


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
#include "miniz.c"

// stb_rect_pack
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"



// Total Annihilation source files.
#include "ta_platform_layer.c"
#include "ta_graphics.c"
#include "ta_game.c"
#include "ta_fs.c"
#include "ta_hpi.c"
#include "ta_gaf.c"
#include "ta_tnt.c"
#include "ta_config.c"

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