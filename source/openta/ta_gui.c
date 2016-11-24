// Copyright (C) 2016 David Reid. See included LICENSE file.

TA_INLINE char* ta_gui__copy_string_prop(char** ppNextStr, const char* src)
{
    char* pNextStr = *ppNextStr;
    size_t srcLen = ta_strlen_or_zero(src);
    if (src != NULL) {
        memcpy(pNextStr, src, srcLen+1);
    } else {
        pNextStr[0] = '\0';
    }

    *ppNextStr += srcLen+1;
    return pNextStr;
}

ta_result ta_gui_load(ta_game* pGame, const char* filePath, ta_gui* pGUI)
{
    if (pGUI == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pGUI);

    if (pGame == NULL || filePath == NULL) return TA_INVALID_ARGS;
    pGUI->pGame = pGame;
    pGUI->heldGadgetIndex = (ta_uint32)-1;
    pGUI->hoveredGadgetIndex = (ta_uint32)-1;
    pGUI->focusedGadgetIndex = (ta_uint32)-1;

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

                payloadSize += ta_strlen_or_zero(ta_config_get_string(pCommonObj, "name"))+1;
                payloadSize += ta_strlen_or_zero(ta_config_get_string(pCommonObj, "help"))+1;

                switch (id)
                {
                    case TA_GUI_GADGET_TYPE_ROOT:
                    {
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "panel"))+1;
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "crdefault"))+1;
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "escdefault"))+1;
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "defaultfocus"))+1;
                    } break;

                    case TA_GUI_GADGET_TYPE_BUTTON:
                    {
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "text"))+1;
                    } break;

                    case TA_GUI_GADGET_TYPE_LABEL:
                    {
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "text"))+1;
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "link"))+1;
                    } break;

                    case TA_GUI_GADGET_TYPE_FONT:
                    {
                        payloadSize += ta_strlen_or_zero(ta_config_get_string(pGadgetObj, "filename"))+1;
                    } break;

                    default: break;
                }
            }

            // TODO: Basic validation.
        }
    }

    pGUI->_pPayload = (ta_uint8*)calloc(1, payloadSize);
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

                    // For multi-stage buttons, the text for each stage is separated with a '|' character. To make rendering easier we will
                    // replace '|' characters with '\0'.
                    dr_string_replace_ascii(pGadget->button.text, '|', '\0');
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


    // Default focus.
    if (!ta_is_string_null_or_empty(pGUI->pGadgets[0].root.defaultfocus)) {
        ta_gui_find_gadget_by_name(pGUI, pGUI->pGadgets[0].root.defaultfocus, &pGUI->focusedGadgetIndex);
    }
    


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
                    if (pGadget->button.stages == 0) {
                        if (ta_common_gui_get_button_frame(&pGame->commonGUI, pGadget->width, pGadget->height, &pGadget->button.iBackgroundFrame) == TA_SUCCESS) {
                            pGadget->button.pBackgroundTextureGroup = &pGame->commonGUI.textureGroup;
                        }
                    } else {
                        if (ta_common_gui_get_multistage_button_frame(&pGame->commonGUI, pGadget->button.stages, &pGadget->button.iBackgroundFrame) == TA_SUCCESS) {
                            pGadget->button.pBackgroundTextureGroup = &pGame->commonGUI.textureGroup;
                        }
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


ta_result ta_gui_get_screen_mapping(ta_gui* pGUI, ta_uint32 screenSizeX, ta_uint32 screenSizeY, float* pScale, float* pOffsetX, float* pOffsetY)
{
    if (pScale) *pScale = 1;
    if (pOffsetX) *pOffsetX = 0;
    if (pOffsetY) *pOffsetY = 0;
    if (pGUI == NULL) return TA_INVALID_ARGS;

    float screenSizeXF = (float)screenSizeX;
    float screenSizeYF = (float)screenSizeY;

    float scaleX = screenSizeXF/640;
    float scaleY = screenSizeYF/480;

    if (scaleX > scaleY) {
        if (pScale) *pScale = scaleY;
        if (pOffsetX) *pOffsetX = (screenSizeXF - (640 * scaleY)) / 2.0f;
    } else {
        if (pScale) *pScale = scaleX;
        if (pOffsetY) *pOffsetY = (screenSizeYF - (480 * scaleX)) / 2.0f;
    }

    return TA_SUCCESS;
}

ta_result ta_gui_map_screen_position(ta_gui* pGUI, ta_uint32 screenSizeX, ta_uint32 screenSizeY, ta_int32 screenPosX, ta_int32 screenPosY, ta_int32* pGUIPosX, ta_int32* pGUIPosY)
{
    if (pGUIPosX) *pGUIPosX = screenPosX;
    if (pGUIPosY) *pGUIPosY = screenPosY;
    if (pGUI == NULL) return TA_INVALID_ARGS;

    float scale;
    float offsetX;
    float offsetY;
    ta_gui_get_screen_mapping(pGUI, screenSizeX, screenSizeY, &scale, &offsetX, &offsetY);

    if (pGUIPosX) *pGUIPosX = (ta_int32)((screenPosX - offsetX) / scale);
    if (pGUIPosY) *pGUIPosY = (ta_int32)((screenPosY - offsetY) / scale);
    return TA_SUCCESS;
}

ta_bool32 ta_gui_get_gadget_under_point(ta_gui* pGUI, ta_int32 posX, ta_int32 posY, ta_uint32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (ta_uint32)-1;
    if (pGUI == NULL) return TA_FALSE;

    // Iterate backwards for this. Reason for this is that GUI elements are rendered start to end which means
    // the elements at the end of the list are at the top of the z-order.
    for (ta_uint32 iGadget = pGUI->gadgetCount; iGadget > 0; --iGadget) {
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget-1];
        if (posX >= pGadget->xpos && posX < pGadget->xpos+pGadget->width &&
            posY >= pGadget->ypos && posY < pGadget->ypos+pGadget->height) {
            if (pGadgetIndex) *pGadgetIndex = iGadget-1;
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}


ta_result ta_gui_hold_gadget(ta_gui* pGUI, ta_uint32 gadgetIndex, ta_uint32 mouseButton)
{
    if (pGUI == NULL || gadgetIndex >= pGUI->gadgetCount) return TA_INVALID_ARGS;
    if (pGUI->heldGadgetIndex != (ta_uint32)-1) {
        return TA_ERROR;    // Another gadget is already being held.
    }

    pGUI->pGadgets[gadgetIndex].isHeld = TA_TRUE;
    pGUI->pGadgets[gadgetIndex].heldMB = mouseButton;
    pGUI->heldGadgetIndex = gadgetIndex;

    return TA_SUCCESS;
}

ta_result ta_gui_release_hold(ta_gui* pGUI, ta_uint32 gadgetIndex)
{
    if (pGUI == NULL || gadgetIndex >= pGUI->gadgetCount) return TA_INVALID_ARGS;
    pGUI->pGadgets[gadgetIndex].isHeld = TA_FALSE;
    pGUI->pGadgets[gadgetIndex].heldMB = 0;
    pGUI->heldGadgetIndex = (ta_uint32)-1;
    
    return TA_SUCCESS;
}

ta_bool32 ta_gui_get_held_gadget(ta_gui* pGUI, ta_uint32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (ta_uint32)-1;
    if (pGUI == NULL) return TA_FALSE;

    if (pGadgetIndex) *pGadgetIndex = pGUI->heldGadgetIndex;
    return pGUI->heldGadgetIndex != (ta_uint32)-1;
}

ta_bool32 ta_gui_find_gadget_by_name(ta_gui* pGUI, const char* name, ta_uint32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (ta_uint32)-1;
    if (pGUI == NULL) return TA_FALSE;

    for (ta_uint32 iGadget = 0; iGadget < pGUI->gadgetCount; ++iGadget) {
        if (_stricmp(pGUI->pGadgets[iGadget].name, name) == 0) {
            if (pGadgetIndex) *pGadgetIndex = iGadget;
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}

ta_bool32 ta_gui_get_focused_gadget(ta_gui* pGUI, ta_uint32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (ta_uint32)-1;
    if (pGUI == NULL) return TA_FALSE;

    if (pGadgetIndex) *pGadgetIndex = pGUI->focusedGadgetIndex;
    return pGUI->focusedGadgetIndex != (ta_uint32)-1;
}

ta_bool32 ta_gui__can_gadget_receive_focus(ta_gui_gadget* pGadget)
{
    assert(pGadget != NULL);

    return pGadget->active != 0 &&
        ((pGadget->id == TA_GUI_GADGET_TYPE_BUTTON && !pGadget->button.grayedout && (pGadget->attribs & TA_GUI_GADGET_ATTRIB_SKIP_FOCUS) == 0) ||
         (pGadget->id == TA_GUI_GADGET_TYPE_LISTBOX) ||
         (pGadget->id == TA_GUI_GADGET_TYPE_TEXTBOX));
}

void ta_gui_focus_next_gadget(ta_gui* pGUI)
{
    if (pGUI == NULL || pGUI->gadgetCount == 0) return;
    
    ta_bool32 wasAnythingFocusedBeforehand = pGUI->focusedGadgetIndex != (ta_uint32)-1; // <-- Only used to avoid infinite recursion.

    // We just keep looping over each gadget until we find one that can receive focus. If we reached the end we just
    // loop back to the start and try again, making sure we don't get stuck in an infinite recursion loop.
    for (ta_uint32 iGadget = (wasAnythingFocusedBeforehand) ? pGUI->focusedGadgetIndex+1 : 1; iGadget < pGUI->gadgetCount; ++iGadget) { // <-- Don't include the root gadget.
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget];
        if (ta_gui__can_gadget_receive_focus(pGadget)) {
            pGUI->focusedGadgetIndex = iGadget;
            return;
        }
    }

    // If we get here it means we weren't able to find a new gadget to focus. In this case we just loop back to the start and try again.
    if (wasAnythingFocusedBeforehand) {
        pGUI->focusedGadgetIndex = (ta_uint32)-1;
        ta_gui_focus_next_gadget(pGUI);
    }
}

void ta_gui_focus_prev_gadget(ta_gui* pGUI)
{
    if (pGUI == NULL || pGUI->gadgetCount == 0) return;
    
    // Everything works the same as ta_gui_focus_next_gadget(), only in reverse.
    ta_bool32 wasAnythingFocusedBeforehand = pGUI->focusedGadgetIndex != (ta_uint32)-1; // <-- Only used to avoid infinite recursion.

    for (ta_uint32 iGadget = (wasAnythingFocusedBeforehand) ? pGUI->focusedGadgetIndex : pGUI->gadgetCount; iGadget > 1; --iGadget) {   // <-- Don't include the root gadget.
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget-1];
        if (ta_gui__can_gadget_receive_focus(pGadget)) {
            pGUI->focusedGadgetIndex = iGadget-1;
            return;
        }
    }

    if (wasAnythingFocusedBeforehand) {
        pGUI->focusedGadgetIndex = (ta_uint32)-1;
        ta_gui_focus_prev_gadget(pGUI);
    }
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

    // The selection logic might be able to be improved here...
    ta_int32 diffXClosest = INT32_MAX;
    ta_int32 diffYClosest = INT32_MAX;
    ta_uint32 iClosestButton = (ta_uint32)-1;
    for (ta_uint32 i = 0; i < ta_countof(pCommonGUI->buttons); ++i) {
        ta_int32 diffX = (ta_int32)pCommonGUI->buttons[i].sizeX - (ta_int32)width;
        ta_int32 diffY = (ta_int32)pCommonGUI->buttons[i].sizeY - (ta_int32)height;
        if (diffX == 0 && diffY == 0) {
            if (pFrameIndex) *pFrameIndex = pCommonGUI->buttons[i].frameIndex;
            return TA_SUCCESS;
        }

        if (ta_abs((ta_int64)diffXClosest+(ta_int64)diffYClosest) > ta_abs((ta_int64)diffX+(ta_int64)diffY)) {
            diffXClosest = diffX;
            diffYClosest = diffY;
            iClosestButton = i;
        }
    }

    if (pFrameIndex) *pFrameIndex = pCommonGUI->buttons[iClosestButton].frameIndex;
    return TA_SUCCESS;
}

ta_result ta_common_gui_get_multistage_button_frame(ta_common_gui* pCommonGUI, ta_uint32 stages, ta_uint32* pFrameIndex)
{
    if (pCommonGUI == NULL || stages > 4 || stages < 1) return TA_INVALID_ARGS;

    const char* sequenceName = NULL;
    if (stages == 1) {
        sequenceName = "stagebuttn1";
    } else if (stages == 2) {
        sequenceName = "stagebuttn2";
    } else if (stages == 3) {
        sequenceName = "stagebuttn3";
    } else {
        assert(stages == 4);
        sequenceName = "stagebuttn4";
    }

    ta_uint32 iSequence;
    if (!ta_gaf_texture_group_find_sequence_by_name(&pCommonGUI->textureGroup, sequenceName, &iSequence)) {
        return TA_RESOURCE_NOT_FOUND;
    }

    if (pFrameIndex) *pFrameIndex = pCommonGUI->textureGroup.pSequences[iSequence].firstFrameIndex;
    return TA_SUCCESS;
}