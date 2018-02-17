// Copyright (C) 2017 David Reid. See included LICENSE file.

typedef struct
{
    ta_game* pGame;
    mal_context contextMAL;
    mal_device playbackDevice;
} ta_audio_context;

ta_audio_context* ta_create_audio_context(ta_game* pGame);
void ta_delete_audio_context(ta_audio_context* pContext);