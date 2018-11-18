// Copyright (C) 2018 David Reid. See included LICENSE file.

#include "taEngine.h"

// miniz
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO
#include "../../external/miniz.c"

#if 0
// Bob Jenkins' lookup3 hash
#include "../../external/lookup3.c"
#endif

// stb_stretchy_buffer
#include "../../external/stretchy_buffer.h"


// dr_math
#define DR_MATH_IMPLEMENTATION
#include "../../external/dr_libs/dr_math.h"

// dr_pcx
#define DR_PCX_IMPLEMENTATION
#include "../../external/dr_libs/dr_pcx.h"

// dr_wav
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "../../external/dr_libs/dr_wav.h"

// dr_mp3
#define DR_MP3_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "../../external/dr_libs/dr_mp3.h"

// mini_al
#define MAL_IMPLEMENTATION
#include "../../external/mini_al/mini_al.h"



// TODO: Remove this dependency.
#define DR_IMPLEMENTATION
#include "../../external/dr_libs/old/dr.h"


// Private Options

// The number of tiles making up a chunk, on each dimension.
#define TA_TERRAIN_CHUNK_SIZE   16


// Total Annihilation source files.
#include "taPlatformLayer.c"
#include "taMisc.c"
#include "taFS.c"
#include "taConfig.c"
#include "taString.c"
#include "taPropertyManager.c"
#include "taFeatures.c"
#include "taInput.c"
#include "taTexturePacker.c"
#include "taMeshBuilder.c"
#include "taAudio.c"
#include "taGraphics.c"
#include "taFont.c"
#include "taGAF.c"
#include "ta3DO.c"
#include "taCOB.c"
#include "taGUI.c"
#include "taMap.c"
#include "taEngineContext.c"