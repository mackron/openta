// Copyright (C) 2017 David Reid. See included LICENSE file.

// TODO:
// - Add support for the user to choose which device to use for output.

TA_PRIVATE void taMALSendProc(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    // TODO: Implement me. This is where all of the mixing takes place.
    (void)pDevice;
    (void)pOutput;
    (void)pInput;
    (void)frameCount;
}

taAudioContext* taCreateAudioContext(taEngineContext* pEngine)
{
    if (pEngine == NULL) {
        return NULL;
    }

    taAudioContext* pAudioContext = (taAudioContext*)calloc(1, sizeof(*pAudioContext));
    if (pAudioContext == NULL) {
        return NULL;
    }

    ma_result resultMAL = ma_context_init(NULL, 0, NULL, &pAudioContext->contextMAL);
    if (resultMAL != MA_SUCCESS) {
        free(pAudioContext);
        return NULL;
    }

    ma_device_config playbackDeviceConfig = ma_device_config_init(ma_device_type_playback);
    playbackDeviceConfig.playback.format   = ma_format_f32;
    playbackDeviceConfig.playback.channels = 2;
    playbackDeviceConfig.sampleRate        = 22050;
    playbackDeviceConfig.dataCallback      = taMALSendProc;
    playbackDeviceConfig.pUserData         = pAudioContext;

    resultMAL = ma_device_init(&pAudioContext->contextMAL, &playbackDeviceConfig, &pAudioContext->playbackDevice);
    if (resultMAL != MA_SUCCESS) {
        ma_context_uninit(&pAudioContext->contextMAL);
        free(pAudioContext);
        return NULL;
    }

    // Start playback here.
    resultMAL = ma_device_start(&pAudioContext->playbackDevice);
    if (resultMAL != MA_SUCCESS) {
        ma_device_uninit(&pAudioContext->playbackDevice);
        ma_context_uninit(&pAudioContext->contextMAL);
        free(pAudioContext);
        return NULL;
    }
    

    return pAudioContext;
}

void taDeleteAudioContext(taAudioContext* pAudioContext)
{
    if (pAudioContext == NULL) {
        return;
    }

    ma_device_uninit(&pAudioContext->playbackDevice);
    ma_context_uninit(&pAudioContext->contextMAL);
    free(pAudioContext);
}