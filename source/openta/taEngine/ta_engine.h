// Copyright (C) 2018 David Reid. See included LICENSE file.

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

// External libraries.
#include "../../external/dr_libs/dr_wav.h"
#include "../../external/dr_libs/dr_math.h"
#include "../../external/dr_libs/dr_pcx.h"
#include "../../external/mini_al/mini_al.h"

// TODO: Remove this dependency.
#include "../../external/dr_libs/dr.h"


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
typedef ta_uint8         ta_bool8;
typedef ta_uint32        ta_bool32;
#define TA_TRUE          1
#define TA_FALSE         0

#define ta_zero_object(p) memset((p), 0, sizeof(*(p)))
#define ta_countof(p) (sizeof((p)) / sizeof((p)[0]))

#define TA_PRIVATE static

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
#include "ta_input.h"
#include "ta_platform_layer.h"
#include "ta_texture_packer.h"
#include "ta_mesh_builder.h"
#include "ta_audio.h"
#include "ta_graphics.h"