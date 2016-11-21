// Copyright (C) 2016 David Reid. See included LICENSE file.

TA_INLINE char* ta_gui__copy_string_prop(char** ppNextStr, const char* src)
{
    char* pNextStr = *ppNextStr;
    size_t srcLen = strlen(src);
    memcpy(pNextStr, src, srcLen+1);

    *ppNextStr += srcLen+1;
    return pNextStr;
}

ta_result ta_gui_load(ta_game* pGame, const char* filePath, ta_gui* pGUI)
{
    if (pGUI == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pGUI);

    if (pGame == NULL || filePath == NULL) return TA_INVALID_ARGS;
    pGUI->pGame = pGame;

    // GUI's are loaded from configs.
    ta_config_obj* pConfig = ta_parse_config_from_file(pGame->pFS, filePath);
    if (pConfig == NULL) {
        return TA_FILE_NOT_FOUND;
    }

    // We load the GUI in two passes. The first pass is used for calculating the size of the memory allocation. The second
    // pass is used to fill in the relevant information.

    // PASS #1
    size_t payloadSize = 0;
    for (unsigned int i = 0; i < pConfig->varCount; ++i) {
        if (ta_config_is_subobj_by_index(pConfig, i)) {
            payloadSize += sizeof(ta_gui_gadget);
            pGUI->gadgetCount += 1;

            // The length of each string property needs to be included with the size of the payload, including their null terminator.
            ta_config_obj* pGadgetObj = pConfig->pVars[i].pObject;
            assert(pGadgetObj != NULL);

            ta_config_obj* pCommonObj = ta_config_get_subobj(pGadgetObj, "COMMON");
            if (pCommonObj != NULL) {
                ta_int32 id = ta_config_get_int(pCommonObj, "id");

                payloadSize += strlen(ta_config_get_string(pCommonObj, "name"))+1;
                payloadSize += strlen(ta_config_get_string(pCommonObj, "help"))+1;

                switch (id)
                {
                    case TA_GUI_GADGET_TYPE_ROOT:
                    {
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "panel"))+1;
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "crdefault"))+1;
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "escdefault"))+1;
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "defaultfocus"))+1;
                    } break;

                    case TA_GUI_GADGET_TYPE_BUTTON:
                    {
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "text"))+1;
                    } break;

                    case TA_GUI_GADGET_TYPE_LABEL:
                    {
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "text"))+1;
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "link"))+1;
                    } break;

                    case TA_GUI_GADGET_TYPE_FONT:
                    {
                        payloadSize += strlen(ta_config_get_string(pGadgetObj, "filename"))+1;
                    } break;

                    default: break;
                }
            }

            // TODO: Basic validation.
        }
    }

    pGUI->_pPayload = (ta_uint8*)malloc(payloadSize);
    if (pGUI->_pPayload == NULL) {
        ta_delete_config(pConfig);
        return TA_OUT_OF_MEMORY;
    }

    pGUI->pGadgets = (ta_gui_gadget*)pGUI->_pPayload;
    
    // PASS #2
    char* pNextStr = pGUI->_pPayload + (sizeof(*pGUI->pGadgets) * pGUI->gadgetCount);
    for (unsigned int i = 0; i < pConfig->varCount; ++i) {
        if (ta_config_is_subobj_by_index(pConfig, i)) {
            ta_config_obj* pGadgetObj = pConfig->pVars[i].pObject;
            assert(pGadgetObj != NULL);

            ta_gui_gadget* pGadget = pGUI->pGadgets + i;

            // [COMMON]
            ta_config_obj* pCommonObj = ta_config_get_subobj(pGadgetObj, "COMMON");
            if (pCommonObj != NULL) {
                pGadget->id            = ta_config_get_int(pCommonObj, "id");
                pGadget->assoc         = ta_config_get_int(pCommonObj, "assoc");
                pGadget->name          = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pCommonObj, "name")); 
                pGadget->xpos          = ta_config_get_int(pCommonObj, "xpos");
                pGadget->ypos          = ta_config_get_int(pCommonObj, "ypos");
                pGadget->width         = ta_config_get_int(pCommonObj, "width");
                pGadget->height        = ta_config_get_int(pCommonObj, "height");
                pGadget->attribs       = ta_config_get_int(pCommonObj, "attribs");
                pGadget->colorf        = ta_config_get_int(pCommonObj, "colorf");
                pGadget->colorb        = ta_config_get_int(pCommonObj, "colorb");
                pGadget->texturenumber = ta_config_get_int(pCommonObj, "texturenumber");
                pGadget->fontnumber    = ta_config_get_int(pCommonObj, "fontnumber");
                pGadget->active        = ta_config_get_int(pCommonObj, "active");
                pGadget->commonattribs = ta_config_get_int(pCommonObj, "commonattribs");
                pGadget->help          = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pCommonObj, "help"));
            }

            switch (pGadget->id)
            {
                case TA_GUI_GADGET_TYPE_ROOT:
                {
                    pGadget->root.totalgadgets = ta_config_get_int(pGadgetObj, "totalgadgets");
                    pGadget->root.panel        = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "panel"));
                    pGadget->root.crdefault    = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "crdefault"));
                    pGadget->root.escdefault   = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "escdefault"));
                    pGadget->root.defaultfocus = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "defaultfocus"));

                    // [VERSION]
                    ta_config_obj* pVersionObj = ta_config_get_subobj(pGadgetObj, "VERSION");
                    if (pVersionObj != NULL) {
                        pGadget->root.version.major    = ta_config_get_int(pVersionObj, "major");
                        pGadget->root.version.minor    = ta_config_get_int(pVersionObj, "minor");
                        pGadget->root.version.revision = ta_config_get_int(pVersionObj, "revision");
                    }
                } break;

                case TA_GUI_GADGET_TYPE_BUTTON:
                {
                    pGadget->button.status    = ta_config_get_int(pGadgetObj, "status");
                    pGadget->button.text      = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "text"));
                    pGadget->button.quickkey  = (ta_uint32)ta_config_get_int(pGadgetObj, "quickkey");
                    pGadget->button.grayedout = ta_config_get_bool(pGadgetObj, "grayedout");
                    pGadget->button.stages    = ta_config_get_int(pGadgetObj, "stages");
                } break;

                case TA_GUI_GADGET_TYPE_LISTBOX:
                {
                    // Nothing.
                } break;

                case TA_GUI_GADGET_TYPE_TEXTBOX:
                {
                    pGadget->textbox.maxchars = ta_config_get_int(pGadgetObj, "maxchars");
                } break;

                case TA_GUI_GADGET_TYPE_SCROLLBAR:
                {
                    pGadget->scrollbar.range    = ta_config_get_int(pGadgetObj, "range");
                    pGadget->scrollbar.thick    = ta_config_get_int(pGadgetObj, "thick");
                    pGadget->scrollbar.knobpos  = ta_config_get_int(pGadgetObj, "knobpos");
                    pGadget->scrollbar.knobsize = ta_config_get_int(pGadgetObj, "knobsize");
                } break;

                case TA_GUI_GADGET_TYPE_LABEL:
                {
                    pGadget->label.text = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "text"));
                    pGadget->label.link = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "link"));
                } break;

                case TA_GUI_GADGET_TYPE_SURFACE:
                {
                    pGadget->surface.hotornot = ta_config_get_bool(pGadgetObj, "hotornot");
                } break;

                case TA_GUI_GADGET_TYPE_FONT:
                {
                    pGadget->font.filename = ta_gui__copy_string_prop(&pNextStr, ta_config_get_string(pGadgetObj, "filename"));
                } break;

                case TA_GUI_GADGET_TYPE_PICTURE:
                {
                    // Nothing.
                } break;

                default: break;
            }
        }
    }

    ta_delete_config(pConfig);


    // There might be an associated GAF file in the "anims" directory that contains graphics for buttons and whatnot.
    const char* fileName = drpath_file_name(filePath);
    assert(fileName != NULL);

    char fileNameGAF[TA_MAX_PATH];
    drpath_copy_and_remove_extension(fileNameGAF, sizeof(fileNameGAF), fileName);
    drpath_append_extension(fileNameGAF, sizeof(fileNameGAF), "GAF");

    char filePathGAF[TA_MAX_PATH];
    drpath_copy_and_append(filePathGAF, sizeof(filePathGAF), "anims", fileNameGAF);
    pGUI->hasGAF = ta_gaf_texture_group_init(pGame, filePathGAF, &pGUI->textureGroupGAF) == TA_SUCCESS;


    // I haven't yet found a way to determine the background image to use for GUIs, so for the moment we will hard code these.
    const char* propVal = ta_get_propertyf(pGame, "%s.BACKGROUND", fileName);
    if (propVal != NULL) {
        pGUI->pBackgroundTexture = ta_load_image(pGame, propVal);
    }


    // For some gadgets we'll need to load some graphics. From what I can tell, graphics will be located in the associated
    // GAF file or commongui. When searching for the graphic we first search in the associated GAF file.
    for (ta_uint32 iGadget = 0; iGadget < pGUI->gadgetCount; ++iGadget) {
        ta_gui_gadget* pGadget = pGUI->pGadgets + iGadget;
        switch (pGadget->id)
        {
            case TA_GUI_GADGET_TYPE_BUTTON:
            {
                ta_uint32 iSequence;
                if (pGUI->hasGAF && ta_gaf_texture_group_find_sequence_by_name(&pGUI->textureGroupGAF, pGadget->name, &iSequence)) {
                    pGadget->button.pBackgroundTextureGroup = &pGUI->textureGroupGAF;
                    pGadget->button.iBackgroundFrame = 0;
                } else {
                    if (ta_common_gui_get_button_frame(&pGame->commonGUI, pGadget->width, pGadget->height, &pGadget->button.iBackgroundFrame) == TA_SUCCESS) {
                        pGadget->button.pBackgroundTextureGroup = &pGame->commonGUI.textureGroup;
                    }
                }
            } break;

            default: break;
        }
    }

    return TA_SUCCESS;
}

ta_result ta_gui_unload(ta_gui* pGUI)
{
    if (pGUI == NULL) return TA_INVALID_ARGS;

    if (pGUI->pBackgroundTexture) ta_delete_texture(pGUI->pBackgroundTexture);
    if (pGUI->hasGAF) ta_gaf_texture_group_uninit(&pGUI->textureGroupGAF);
    free(pGUI->_pPayload);

    return TA_SUCCESS;
}


// Common GUI
// ==========
ta_result ta_common_gui__create_texture_atlas(ta_game* pGame, ta_common_gui* pCommonGUI, ta_texture_packer* pPacker, ta_texture** ppTexture)
{
    assert(pGame != NULL);
    assert(pCommonGUI != NULL);
    assert(pPacker != NULL);
    assert(ppTexture != NULL);
    
    *ppTexture = ta_create_texture(pGame->pGraphics, pPacker->width, pPacker->height, 1, pPacker->pImageData);
    if (*ppTexture == NULL) {
        return TA_FAILED_TO_CREATE_RESOURCE;
    }

    return TA_SUCCESS;
}

ta_result ta_common_gui_load(ta_game* pGame, ta_common_gui* pCommonGUI)
{
    if (pCommonGUI == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pCommonGUI);

    pCommonGUI->pGame = pGame;

    ta_result result = ta_gaf_texture_group_init(pGame, "anims/commongui.GAF", &pCommonGUI->textureGroup);
    if (result != TA_SUCCESS) {
        return result;
    }

    ta_uint32 iSequence;
    if (ta_gaf_texture_group_find_sequence_by_name(&pCommonGUI->textureGroup, "BUTTONS0", &iSequence)) {
        ta_gaf_texture_group_sequence* pSequence = pCommonGUI->textureGroup.pSequences + iSequence;

        pCommonGUI->buttons[0].sizeX = 321;
        pCommonGUI->buttons[0].sizeY = 20;
        pCommonGUI->buttons[0].frameIndex = pSequence->firstFrameIndex + 4;

        pCommonGUI->buttons[1].sizeX = 120;
        pCommonGUI->buttons[1].sizeY = 20;
        pCommonGUI->buttons[1].frameIndex = pSequence->firstFrameIndex + 8;

        pCommonGUI->buttons[2].sizeX = 96;
        pCommonGUI->buttons[2].sizeY = 20;
        pCommonGUI->buttons[2].frameIndex = pSequence->firstFrameIndex + 12;

        pCommonGUI->buttons[3].sizeX = 112;
        pCommonGUI->buttons[3].sizeY = 20;
        pCommonGUI->buttons[3].frameIndex = pSequence->firstFrameIndex + 16;

        pCommonGUI->buttons[4].sizeX = 80;
        pCommonGUI->buttons[4].sizeY = 20;
        pCommonGUI->buttons[4].frameIndex = pSequence->firstFrameIndex + 20;

        pCommonGUI->buttons[5].sizeX = 96;
        pCommonGUI->buttons[5].sizeY = 31;
        pCommonGUI->buttons[5].frameIndex = pSequence->firstFrameIndex + 24;
    }

    return TA_SUCCESS;
}

ta_result ta_common_gui_unload(ta_common_gui* pCommonGUI)
{
    if (pCommonGUI == NULL) return TA_INVALID_ARGS;

    ta_gaf_texture_group_uninit(&pCommonGUI->textureGroup);
    return TA_SUCCESS;
}

ta_result ta_common_gui_get_button_frame(ta_common_gui* pCommonGUI, ta_uint32 width, ta_uint32 height, ta_uint32* pFrameIndex)
{
    if (pCommonGUI == NULL) return TA_INVALID_ARGS;

    for (int i = 0; i < ta_countof(pCommonGUI->buttons); ++i) {
        if (pCommonGUI->buttons[i].sizeX == width && pCommonGUI->buttons[i].sizeY == height) {
            if (pFrameIndex) *pFrameIndex = pCommonGUI->buttons[i].frameIndex;
            return TA_SUCCESS;
        }
    }

    return TA_ERROR;
}