// Copyright (C) 2017 David Reid. See included LICENSE file.

typedef struct
{
    taEngineContext* pEngine;
    mal_context contextMAL;
    mal_device playbackDevice;
} taAudioContext;

taAudioContext* taCreateAudioContext(taEngineContext* pEngine);
void taDeleteAudioContext(taAudioContext* pContext);
