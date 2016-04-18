// Public domain. See "unlicense" statement at the end of this file.

struct ta_game
{
    // The command line that was used to start up the game.
    dr_cmdline cmdline;

    // The graphics context.
    ta_graphics_context* pGraphics;

    // The game window.
    ta_window* pWindow;

    // The game timer for stepping the game.
    ta_timer* pTimer;
};

// Creates a game instance.
ta_game* ta_create_game(dr_cmdline cmdline);

// Deletes a game instance.
void ta_delete_game(ta_game* pGame);


// Runs the given game.
int ta_game_run(ta_game* pGame);

// Steps and renders a single frame.
void ta_do_frame(ta_game* pGame);


// Steps the game.
void ta_game_step(ta_game* pGame);

// Renders the game.
void ta_game_render(ta_game* pGame);