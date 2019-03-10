// Copyright (C) 2017 David Reid. See included LICENSE file.

typedef struct
{
    taEngineContext* pEngine;
    ma_context contextMAL;
    ma_device playbackDevice;
} taAudioContext;

taAudioContext* taCreateAudioContext(taEngineContext* pEngine);
void taDeleteAudioContext(taAudioContext* pAudioContext);
