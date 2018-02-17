// Copyright (C) 2016 David Reid. See included LICENSE file.

// miniz
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO
#include "../../external/miniz.c"

// Bob Jenkins' lookup3 hash
#include "../../external/lookup3.c"

// stb_stretchy_buffer
#include "../../external/stretchy_buffer.h"


// dr_wav
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "../../external/dr_libs/dr_wav.h"

// dr_math
#define DR_MATH_IMPLEMENTATION
#include "../../external/dr_libs/dr_math.h"

// dr_pcx
#define DR_PCX_IMPLEMENTATION
#include "../../external/dr_libs/dr_pcx.h"

// mini_al
#define MAL_IMPLEMENTATION
#include "../../external/mini_al/mini_al.h"



// TODO: Remove this dependency.
#define DR_IMPLEMENTATION
#include "../../external/dr_libs/dr.h"


// Private Options

// The number of tiles making up a chunk, on each dimension.
#define TA_TERRAIN_CHUNK_SIZE   16


// Total Annihilation source files.
#include "ta_misc.c"
#include "ta_string.c"
#include "ta_input.c"
#include "ta_platform_layer.c"
#include "ta_texture_packer.c"
#include "ta_mesh_builder.c"
#include "ta_audio.c"
#include "ta_graphics.c"