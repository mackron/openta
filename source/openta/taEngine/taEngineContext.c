// Copyright (C) 2018 David Reid. See included LICENSE file.

TA_PRIVATE taBool32 taLoadPalette(taFS* pFS, const char* filePath, taUInt32* paletteOut)
{
    taFile* pPaletteFile = taOpenFile(pFS, filePath, 0);
    if (pPaletteFile == NULL) {
        return TA_FALSE;
    }

    size_t bytesRead;
    if (!taReadFile(pPaletteFile, paletteOut, 1024, &bytesRead) || bytesRead != 1024) {
        return TA_FALSE;
    }

    taCloseFile(pPaletteFile);


    // The palettes will include room for an alpha component, but it'll always be set to 0 (fully transparent). However, due
    // to the way I'm doing things in this project it's better for us to convert all of them to fully opaque.
    for (int i = 0; i < 256; ++i) {
        paletteOut[i] |= 0xFF000000;
    }

    return TA_TRUE;
}


taResult taEngineContextInit(int argc, char** argv, taLoadPropertiesProc onLoadProperties, taStepProc onStep, void* pUserData, taEngineContext* pEngine)
{
    if (pEngine == NULL) return TA_INVALID_ARGS;
    taZeroObject(pEngine);
    pEngine->argc = argc;
    pEngine->argv = argv;
    pEngine->onLoadProperties = onLoadProperties;
    pEngine->onStep = onStep;
    pEngine->pUserData = pUserData;

    taResult result = TA_SUCCESS;


    //// Property Manager ////
    //
    // Properties are loaded first because we may want to use them for initialization of other components.
    result = ta_property_manager_init(&pEngine->properties);
    if (result != TA_SUCCESS) {
        goto on_error0;
    }

    if (onLoadProperties) {
        onLoadProperties(pEngine, &pEngine->properties);
    }



    //// File System ////
    pEngine->pFS = taCreateFileSystem();
    if (pEngine->pFS == NULL) {
        result = TA_ERROR;
        goto on_error1;
    }


    //// Graphics ////

    // The palettes. The graphics system depends on these so they need to be loaded first.
    if (!taLoadPalette(pEngine->pFS, "palettes/PALETTE.PAL", pEngine->palette)) {
        result = TA_ERROR;
        goto on_error2;
    }

    if (!taLoadPalette(pEngine->pFS, "palettes/GUIPAL.PAL", pEngine->guipal)) {
        result = TA_ERROR;
        goto on_error2;
    }

    // Due to the way I'm doing a few things with the rendering, we want to use a specific entry in the palette to act as a
    // fully transparent value. For now I'll be using entry 240. Any non-transparent pixel that wants to use this entry will
    // be changed to use color 0. From what I can tell there should be no difference in visual appearance by doing this.
    pEngine->palette[TA_TRANSPARENT_COLOR] &= 0x00FFFFFF; // <-- Make the alpha component fully transparent.

    pEngine->pGraphics = taCreateGraphicsContext(pEngine, pEngine->palette);
    if (pEngine->pGraphics == NULL) {
        result = TA_ERROR;
        goto on_error2;
    }



    //// Audio ////
    pEngine->pAudio = taCreateAudioContext(pEngine);
    if (pEngine->pAudio == NULL) {
        result = TA_ERROR;
        goto on_error3;
    }



    //// Input ////
    result = taInputStateInit(&pEngine->input);
    if (result != TA_SUCCESS) {
        goto on_error4;
    }



    //// GUI ////

    // There are a few required resources that are hard coded from what I can tell.
    result = taFontLoad(pEngine, "anims/hattfont12.GAF/Haettenschweiler (120)", &pEngine->font);
    if (result != TA_SUCCESS) {
        goto on_error5;
    }

    result = taFontLoad(pEngine, "anims/hattfont11.GAF/Haettenschweiler (120)", &pEngine->fontSmall);
    if (result != TA_SUCCESS) {
        goto on_error6;
    }

    result = taCommonGUILoad(pEngine, &pEngine->commonGUI);
    if (result != TA_SUCCESS) {
        goto on_error7;
    }


    //// Features ////
    pEngine->pFeatures = taCreateFeaturesLibrary(pEngine->pFS);
    if (pEngine->pFeatures == NULL) {
        result = TA_ERROR;
        goto on_error8;
    }


    // Texture GAFs. This is every GAF file contained in the "textures" directory. These are loaded in two passes. The first
    // pass counts the number of GAF files, and the second pass opens them.
    pEngine->textureGAFCount = 0;
    taFSIterator* iGAF = taFSBegin(pEngine->pFS, "textures", TA_FALSE);
    while (taFSNext(iGAF)) {
        if (drpath_extension_equal(iGAF->fileInfo.relativePath, "gaf")) {
            pEngine->textureGAFCount += 1;
        }
    }
    taFSEnd(iGAF);

    pEngine->ppTextureGAFs = (taGAF**)malloc(pEngine->textureGAFCount * sizeof(*pEngine->ppTextureGAFs));
    if (pEngine->ppTextureGAFs == NULL) {
        goto on_error9;  // Failed to load texture GAFs.
    }

    pEngine->textureGAFCount = 0;
    iGAF = taFSBegin(pEngine->pFS, "textures", TA_FALSE);
    while (taFSNext(iGAF)) {
        if (drpath_extension_equal(iGAF->fileInfo.relativePath, "gaf")) {
            pEngine->ppTextureGAFs[pEngine->textureGAFCount] = taOpenGAF(pEngine->pFS, iGAF->fileInfo.relativePath);
            if (pEngine->ppTextureGAFs[pEngine->textureGAFCount] != NULL) {
                pEngine->textureGAFCount += 1;
            } else {
                // Failed to open the GAF file.
                // TODO: Log this as a warning.
            }
        }
    }
    taFSEnd(iGAF);

    

    return TA_SUCCESS;

//on_error11: for (taUInt32 i = 0; i < pEngine->textureGAFCount; ++i) { taCloseGAF(pEngine->ppTextureGAFs[i]); }
//on_error10: free(pEngine->ppTextureGAFs);
on_error9:  taDeleteFeaturesLibrary(pEngine->pFeatures);
on_error8:  taCommonGUIUnload(&pEngine->commonGUI);
on_error7:  taFontUnload(&pEngine->fontSmall);
on_error6:  taFontUnload(&pEngine->font);
on_error5:  taInputStateUninit(&pEngine->input);
on_error4:  taDeleteAudioContext(pEngine->pAudio);
on_error3:  taDeleteGraphicsContext(pEngine->pGraphics);
on_error2:  taDeleteFileSystem(pEngine->pFS);
on_error1:  ta_property_manager_uninit(&pEngine->properties);
on_error0:
    return result;
}

taResult taEngineContextUninit(taEngineContext* pEngine)
{
    if (pEngine == NULL) {
        return TA_INVALID_ARGS;
    }

    for (taUInt32 i = 0; i < pEngine->textureGAFCount; ++i) { taCloseGAF(pEngine->ppTextureGAFs[i]); }
    free(pEngine->ppTextureGAFs);
    taDeleteFeaturesLibrary(pEngine->pFeatures);
    taCommonGUIUnload(&pEngine->commonGUI);
    taFontUnload(&pEngine->fontSmall);
    taFontUnload(&pEngine->font);
    taInputStateUninit(&pEngine->input);
    taDeleteAudioContext(pEngine->pAudio);
    taDeleteGraphicsContext(pEngine->pGraphics);
    taDeleteFileSystem(pEngine->pFS);
    ta_property_manager_uninit(&pEngine->properties);
    return TA_SUCCESS;
}



taTexture* taLoadImage(taEngineContext* pEngine, const char* filePath)
{
    if (pEngine == NULL || filePath == NULL) {
        return NULL;
    }
    
    if (drpath_extension_equal(filePath, "pcx")) {
        taFile* pFile = taOpenFile(pEngine->pFS, filePath, 0);
        if (pFile == NULL) {
            return NULL;    // File not found.
        }

        int width;
        int height;
        dr_uint8* pImageData = drpcx_load_memory(pFile->pFileData, pFile->sizeInBytes, TA_FALSE, &width, &height, NULL, 4);
        if (pImageData == NULL) {
            return NULL;    // Not a valid PCX file.
        }

        taTexture* pTexture = taCreateTexture(pEngine->pGraphics, (unsigned int)width, (unsigned int)height, 4, pImageData);
        if (pTexture == NULL) {
            drpcx_free(pImageData);
            return NULL;    // Failed to create texture.
        }

        drpcx_free(pImageData);
        return pTexture;
    }

    // Failed to open file.
    return NULL;
}



void taCaptureMouse(taWindow* pWindow)
{
    taWindowCaptureMouse(pWindow);
}

void taReleaseMouse(taEngineContext* pEngine)
{
    (void)pEngine;
    taWindowReleaseMouse();
}


//// Events from Window

void taOnWindowSize(taEngineContext* pEngine, unsigned int newWidth, unsigned int newHeight)
{
    assert(pEngine != NULL);
    taSetResolution(pEngine->pGraphics, newWidth, newHeight);
}

void taOnMouseButtonDown(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    taInputStateOnMouseButtonDown(&pEngine->input, button);
    //taCaptureMouse(pEngine);
}

void taOnMouseButtonUp(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)posX;
    (void)posY;
    (void)stateFlags;

    taInputStateOnMouseButtonUp(&pEngine->input, button);

    //if (!taInputStateIsAnyMouseButtonDown(&pEngine->input)) {
    //    taReleaseMouse(pEngine);
    //}
}

void taOnMouseButtonDblClick(taEngineContext* pEngine, int button, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)pEngine;
    (void)button;
    (void)posX;
    (void)posY;
    (void)stateFlags;
}

void taOnMouseWheel(taEngineContext* pEngine, int delta, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)pEngine;
    (void)delta;
    (void)posX;
    (void)posY;
    (void)stateFlags;
}

void taOnMouseMove(taEngineContext* pEngine, int posX, int posY, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)stateFlags;

    taInputStateOnMouseMove(&pEngine->input, (float)posX, (float)posY);
}

void taOnMouseEnter(taEngineContext* pEngine)
{
    assert(pEngine != NULL);
    (void)pEngine;
}

void taOnMouseLeave(taEngineContext* pEngine)
{
    assert(pEngine != NULL);
    (void)pEngine;
}

void taOnKeyDown(taEngineContext* pEngine, taKey key, unsigned int stateFlags)
{
    assert(pEngine != NULL);

    if (key < taCountOf(pEngine->input.keyState)) {
        if ((stateFlags & TA_KEY_STATE_AUTO_REPEATED) == 0) {
            taInputStateOnKeyDown(&pEngine->input, key);
        }
    }
}

void taOnKeyUp(taEngineContext* pEngine, taKey key, unsigned int stateFlags)
{
    assert(pEngine != NULL);

    if (key < taCountOf(pEngine->input.keyState)) {
        taInputStateOnKeyUp(&pEngine->input, key);
    }
}

void taOnPrintableKeyDown(taEngineContext* pEngine, taUInt32 utf32, unsigned int stateFlags)
{
    assert(pEngine != NULL);
    (void)pEngine;
    (void)utf32;
    (void)stateFlags;
}
