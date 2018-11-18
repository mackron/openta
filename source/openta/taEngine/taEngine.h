// Copyright (C) 2018 David Reid. See included LICENSE file.

#ifndef TA_ENGINE_H
#define TA_ENGINE_H

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
#include "../../external/dr_libs/dr_math.h"
#include "../../external/dr_libs/dr_pcx.h"
#include "../../external/dr_libs/dr_wav.h"
#include "../../external/mini_al/mini_al.h"

// TODO: Remove this dependency.
#include "../../external/dr_libs/old/dr.h"


// Options.

// The color within the palette to use as the transparent pixel.
#define TA_TRANSPARENT_COLOR 240

// The maximum length of a path. This should rarely need to be increased, but try increasing it if you encounter truncation issues.
#define TA_MAX_PATH 256

// The maximum length of a feature name.
#define TA_MAX_FEATURE_NAME 64


// Basic types.
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef   signed char    taInt8;
typedef unsigned char    taUInt8;
typedef   signed short   taInt16;
typedef unsigned short   taUInt16;
typedef   signed int     taInt32;
typedef unsigned int     taUInt32;
typedef   signed __int64 taInt64;
typedef unsigned __int64 taUInt64;
#else
#include <stdint.h>
typedef int8_t           taInt8;
typedef uint8_t          taUInt8;
typedef int16_t          taInt16;
typedef uint16_t         taUInt16;
typedef int32_t          taInt32;
typedef uint32_t         taUInt32;
typedef int64_t          taInt64;
typedef uint64_t         taUInt64;
#endif
typedef taUInt8          taBool8;
typedef taUInt32         taBool32;
#define TA_TRUE          1
#define TA_FALSE         0

#define taZeroObject(p) memset((p), 0, sizeof(*(p)))
#define taCountOf(p) (sizeof((p)) / sizeof((p)[0]))

#define TA_PRIVATE static

// The maximum size of the texture atlas. We don't really want to use the GPU's maximum texture size
// because it can result in excessive wastage - modern GPUs support 16K textures, which is much more
// than we need and it's better to not needlessly waste the player's system resources. Keep this at
// a power of 2.
#define TA_MAX_TEXTURE_ATLAS_SIZE   512 /*4096*/

typedef struct taEngineContext taEngineContext;
typedef struct ta_graphics_context ta_graphics_context;
typedef struct ta_font ta_font;
typedef enum ta_seek_origin ta_seek_origin;

// Total Annihilation headers.
#include "taErrors.h"
#include "taPlatformLayer.h"
#include "taMisc.h"
#include "taFS.h"
#include "taConfig.h"
#include "taTypeDeclarations.h"
#include "taString.h"
#include "taPropertyManager.h"
#include "taFeatures.h"
#include "taInput.h"
#include "taTexturePacker.h"
#include "taMeshBuilder.h"
#include "taAudio.h"
#include "taGraphics.h"
#include "taFont.h"
#include "taGAF.h"
#include "ta3DO.h"
#include "taCOB.h"
#include "taGUI.h"
#include "taMap.h"
#include "taEngineContext.h"

#endif