// Copyright (C) 2017 David Reid. See included LICENSE file.

// TODO:
// - Add support for the user to choose which device to use for output.

TA_PRIVATE mal_uint32 ta_mal_send_proc(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
{
    // TODO: Implement me. This is where all of the mixing takes place.
    (void)pDevice;
    (void)frameCount;
    (void)pSamples;

    return 0;
}

ta_audio_context* ta_create_audio_context(ta_game* pGame)
{
    if (pGame == NULL) return NULL;

    ta_audio_context* pContext = (ta_audio_context*)calloc(1, sizeof(*pContext));
    if (pContext == NULL) {
        return NULL;
    }

    mal_result resultMAL = mal_context_init(NULL, 0, NULL, &pContext->contextMAL);
    if (resultMAL != MAL_SUCCESS) {
        free(pContext);
        return NULL;
    }

    mal_device_config playbackDeviceConfig = mal_device_config_init_playback(mal_format_f32, 2, 22050, ta_mal_send_proc);

    resultMAL = mal_device_init(&pContext->contextMAL, mal_device_type_playback, NULL, &playbackDeviceConfig, pContext, &pContext->playbackDevice);
    if (resultMAL != MAL_SUCCESS) {
        mal_context_uninit(&pContext->contextMAL);
        free(pContext);
        return NULL;
    }

    // Start playback here.
    resultMAL = mal_device_start(&pContext->playbackDevice);
    if (resultMAL != MAL_SUCCESS) {
        mal_device_uninit(&pContext->playbackDevice);
        mal_context_uninit(&pContext->contextMAL);
        free(pContext);
        return NULL;
    }
    

    return pContext;
}

void ta_delete_audio_context(ta_audio_context* pContext)
{
    if (pContext == NULL) return;

    mal_device_uninit(&pContext->playbackDevice);
    mal_context_uninit(&pContext->contextMAL);
    free(pContext);
}