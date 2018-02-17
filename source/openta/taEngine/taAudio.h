// Copyright (C) 2017 David Reid. See included LICENSE file.

typedef struct
{
    taEngineContext* pEngine;
    mal_context contextMAL;
    mal_device playbackDevice;
} ta_audio_context;

ta_audio_context* ta_create_audio_context(taEngineContext* pEngine);
void ta_delete_audio_context(ta_audio_context* pContext);