// Copyright (C) 2018 David Reid. See included LICENSE file.

// The main file containing the entry point. This is the only compiled file for the entire game.

#include "taEngine/taEngine.h"


#include "taGame/taFont.h"
#include "taGame/taGAF.h"
#include "taGame/taGUI.h"
#include "taGame/taPropertyManager.h"
#include "taGame/taGame.h"
#include "taGame/taFS.h"
#include "taGame/ta3DO.h"
#include "taGame/taConfig.h"
#include "taGame/taFeatures.h"
#include "taGame/taMap.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// IMPLEMENTATION
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "taEngine/taEngine.c"

#include "taGame/taFont.c"
#include "taGame/taGAF.c"
#include "taGame/taGUI.c"
#include "taGame/taPropertyManager.c"
#include "taGame/taGame.c"
#include "taGame/taFS.c"
#include "taGame/ta3DO.c"
#include "taGame/taConfig.c"
#include "taGame/taFeatures.c"
#include "taGame/taMap.c"

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