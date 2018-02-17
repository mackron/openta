// Copyright (C) 2018 David Reid. See included LICENSE file.

// The main file containing the entry point. This is the only compiled file for the entire game.

#include "taEngine/ta_engine.h"


#include "taGame/ta_font.h"
#include "taGame/ta_gaf.h"
#include "taGame/ta_gui.h"
#include "taGame/ta_property_manager.h"
#include "taGame/ta_game.h"
#include "taGame/ta_fs.h"
#include "taGame/ta_3do.h"
#include "taGame/ta_config.h"
#include "taGame/ta_features.h"
#include "taGame/ta_map.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "taEngine/ta_engine.c"

#include "taGame/ta_font.c"
#include "taGame/ta_gaf.c"
#include "taGame/ta_gui.c"
#include "taGame/ta_property_manager.c"
#include "taGame/ta_game.c"
#include "taGame/ta_fs.c"
#include "taGame/ta_3do.c"
#include "taGame/ta_config.c"
#include "taGame/ta_features.c"
#include "taGame/ta_map.c"

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