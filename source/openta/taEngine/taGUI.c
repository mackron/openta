// Copyright (C) 2018 David Reid. See included LICENSE file.

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

ta_result ta_gui_load(taEngineContext* pEngine, const char* filePath, ta_gui* pGUI)
{
    if (pGUI == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pGUI);

    if (pEngine == NULL || filePath == NULL) return TA_INVALID_ARGS;
    pGUI->pEngine = pEngine;
    pGUI->heldGadgetIndex = (taUInt32)-1;
    pGUI->hoveredGadgetIndex = (taUInt32)-1;
    pGUI->focusedGadgetIndex = (taUInt32)-1;

    // GUI's are loaded from configs.
    ta_config_obj* pConfig = ta_parse_config_from_file(pEngine->pFS, filePath);
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
                taInt32 id = ta_config_get_int(pCommonObj, "id");

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

    pGUI->_pPayload = (taUInt8*)calloc(1, payloadSize);
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
                    pGadget->button.quickkey  = (taUInt32)ta_config_get_int(pGadgetObj, "quickkey");
                    pGadget->button.grayedout = ta_config_get_bool(pGadgetObj, "grayedout");
                    pGadget->button.stages    = ta_config_get_int(pGadgetObj, "stages");

                    // For multi-stage buttons, the text for each stage is separated with a '|' character. To make rendering easier we will
                    // replace '|' characters with '\0'.
                    dr_string_replace_ascii(pGadget->button.text, '|', '\0');
                } break;

                case TA_GUI_GADGET_TYPE_LISTBOX:
                {
                    pGadget->listbox.pItems = NULL;
                    pGadget->listbox.itemCount = 0;
                    pGadget->listbox.iSelectedItem = (taUInt32)-1;
                    pGadget->listbox.scrollPos = 0;
                    pGadget->listbox.pageSize = (taUInt32)max(1, pGadget->height / pEngine->font.height);
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
                    pGadget->label.iLinkedGadget = (taUInt32)-1;
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

    // We need to do some post-processing to link certain gadgets together.

    for (taUInt32 iGadget = 1; iGadget < pGUI->gadgetCount; ++iGadget) {
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget];
        if (pGadget->id == TA_GUI_GADGET_TYPE_LABEL) {
            if (!ta_is_string_null_or_empty(pGadget->label.link)) {
                ta_gui_find_gadget_by_name(pGUI, pGadget->label.link, &pGadget->label.iLinkedGadget);
            }
        }
    }

    


    // There might be an associated GAF file in the "anims" directory that contains graphics for buttons and whatnot.
    const char* fileName = drpath_file_name(filePath);
    assert(fileName != NULL);

    char fileNameGAF[TA_MAX_PATH];
    drpath_copy_and_remove_extension(fileNameGAF, sizeof(fileNameGAF), fileName);
    drpath_append_extension(fileNameGAF, sizeof(fileNameGAF), "GAF");

    char filePathGAF[TA_MAX_PATH];
    drpath_copy_and_append(filePathGAF, sizeof(filePathGAF), "anims", fileNameGAF);
    pGUI->hasGAF = ta_gaf_texture_group_init(pEngine, filePathGAF, ta_color_mode_truecolor, &pGUI->textureGroupGAF) == TA_SUCCESS;


    // I haven't yet found a way to determine the background image to use for GUIs, so for the moment we will hard code these.
    const char* propVal = ta_property_manager_getf(&pEngine->properties, "%s.BACKGROUND", fileName);
    if (propVal != NULL) {
        pGUI->pBackgroundTexture = ta_load_image(pEngine, propVal);
    }


    // For some gadgets we'll need to load some graphics. From what I can tell, graphics will be located in the associated
    // GAF file or commongui. When searching for the graphic we first search in the associated GAF file.
    for (taUInt32 iGadget = 0; iGadget < pGUI->gadgetCount; ++iGadget) {
        ta_gui_gadget* pGadget = pGUI->pGadgets + iGadget;
        switch (pGadget->id)
        {
            case TA_GUI_GADGET_TYPE_BUTTON:
            {
                taUInt32 iSequence;
                if (pGUI->hasGAF && ta_gaf_texture_group_find_sequence_by_name(&pGUI->textureGroupGAF, pGadget->name, &iSequence)) {
                    pGadget->button.pBackgroundTextureGroup = &pGUI->textureGroupGAF;
                    pGadget->button.iBackgroundFrame = pGUI->textureGroupGAF.pSequences[iSequence].firstFrameIndex + pGadget->button.status;
                } else {
                    if (pGadget->button.stages == 0) {
                        if (ta_common_gui_get_button_frame(&pEngine->commonGUI, pGadget->width, pGadget->height, &pGadget->button.iBackgroundFrame) == TA_SUCCESS) {
                            pGadget->button.pBackgroundTextureGroup = &pEngine->commonGUI.textureGroup;
                        }
                    } else {
                        if (ta_common_gui_get_multistage_button_frame(&pEngine->commonGUI, pGadget->button.stages, &pGadget->button.iBackgroundFrame) == TA_SUCCESS) {
                            pGadget->button.pBackgroundTextureGroup = &pEngine->commonGUI.textureGroup;
                        }
                    }
                }

                // Experiment: Change the size of the button to that of it's graphic.
                if (pGadget->button.pBackgroundTextureGroup != NULL) {
                    pGadget->width  = (taInt32)pGadget->button.pBackgroundTextureGroup->pFrames[pGadget->button.iBackgroundFrame].sizeX;
                    pGadget->height = (taInt32)pGadget->button.pBackgroundTextureGroup->pFrames[pGadget->button.iBackgroundFrame].sizeY;
                }
            } break;

            case TA_GUI_GADGET_TYPE_SCROLLBAR:
            {
                pGadget->scrollbar.pTextureGroup = &pEngine->commonGUI.textureGroup;
                if (pGadget->attribs & TA_GUI_SCROLLBAR_TYPE_VERTICAL) {
                    pGadget->scrollbar.iArrow0Frame = pEngine->commonGUI.scrollbar.arrowUpFrameIndex;
                    pGadget->scrollbar.iArrow1Frame = pEngine->commonGUI.scrollbar.arrowDownFrameIndex;
                    pGadget->scrollbar.iTrackBegFrame = pEngine->commonGUI.scrollbar.trackTopFrameIndex;
                    pGadget->scrollbar.iTrackEndFrame = pEngine->commonGUI.scrollbar.trackBottomFrameIndex;
                    pGadget->scrollbar.iTrackMidFrame = pEngine->commonGUI.scrollbar.trackMidVertFrameIndex;
                    pGadget->scrollbar.iThumbFrame = pEngine->commonGUI.scrollbar.thumbVertFrameIndex;
                } else {
                    pGadget->scrollbar.iArrow0Frame = pEngine->commonGUI.scrollbar.arrowLeftFrameIndex;
                    pGadget->scrollbar.iArrow1Frame = pEngine->commonGUI.scrollbar.arrowRightFrameIndex;
                    pGadget->scrollbar.iTrackBegFrame = pEngine->commonGUI.scrollbar.trackLeftFrameIndex;
                    pGadget->scrollbar.iTrackEndFrame = pEngine->commonGUI.scrollbar.trackRightFrameIndex;
                    pGadget->scrollbar.iTrackMidFrame = pEngine->commonGUI.scrollbar.trackMidHorzFrameIndex;
                    pGadget->scrollbar.iThumbFrame = pEngine->commonGUI.scrollbar.thumbHorzFrameIndex;
                }
                
                pGadget->scrollbar.iThumbCapTopFrame = pEngine->commonGUI.scrollbar.thumbCapTopFrameIndex;
                pGadget->scrollbar.iThumbCapBotFrame = pEngine->commonGUI.scrollbar.thumbCapBotFrameIndex;
            } break;

            // TESTING. TODO: DELETE ME.
            case TA_GUI_GADGET_TYPE_LISTBOX:
            {
                const char* pTestItems[] = {
                    "Hello, World!",
                    "Testing 1",
                    "Blah Blah"
                };
                ta_gui_set_listbox_items(pGadget, pTestItems, ta_countof(pTestItems));
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


ta_result ta_gui_get_screen_mapping(ta_gui* pGUI, taUInt32 screenSizeX, taUInt32 screenSizeY, float* pScale, float* pOffsetX, float* pOffsetY)
{
    if (pScale) *pScale = 1;
    if (pOffsetX) *pOffsetX = 0;
    if (pOffsetY) *pOffsetY = 0;
    if (pGUI == NULL) return TA_INVALID_ARGS;

    float screenSizeXF = (float)screenSizeX;
    float screenSizeYF = (float)screenSizeY;

    float scaleX = screenSizeXF/640;
    float scaleY = screenSizeYF/480;

    float scale;
    if (scaleX > scaleY) {
        scale = scaleY;
        if (pOffsetX) *pOffsetX = (screenSizeXF - (640*scale)) / 2.0f;
    } else {
        scale = scaleX;
        if (pOffsetY) *pOffsetY = (screenSizeYF - (480*scale)) / 2.0f;
    }

    if (pScale) *pScale = scale;
    if (pOffsetX) *pOffsetX += (pGUI->pGadgets[0].xpos*scale);
    if (pOffsetY) *pOffsetY += (pGUI->pGadgets[0].ypos*scale);
    return TA_SUCCESS;
}

ta_result ta_gui_map_screen_position(ta_gui* pGUI, taUInt32 screenSizeX, taUInt32 screenSizeY, taInt32 screenPosX, taInt32 screenPosY, taInt32* pGUIPosX, taInt32* pGUIPosY)
{
    if (pGUIPosX) *pGUIPosX = screenPosX;
    if (pGUIPosY) *pGUIPosY = screenPosY;
    if (pGUI == NULL) return TA_INVALID_ARGS;

    float scale;
    float offsetX;
    float offsetY;
    ta_gui_get_screen_mapping(pGUI, screenSizeX, screenSizeY, &scale, &offsetX, &offsetY);

    if (pGUIPosX) *pGUIPosX = (taInt32)((screenPosX - offsetX) / scale);
    if (pGUIPosY) *pGUIPosY = (taInt32)((screenPosY - offsetY) / scale);
    return TA_SUCCESS;
}

taBool32 ta_gui_get_gadget_under_point(ta_gui* pGUI, taInt32 posX, taInt32 posY, taUInt32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (taUInt32)-1;
    if (pGUI == NULL) return TA_FALSE;

    // Iterate backwards for this. Reason for this is that GUI elements are rendered start to end which means
    // the elements at the end of the list are at the top of the z-order.
    for (taUInt32 iGadget = pGUI->gadgetCount; iGadget > 0; --iGadget) {
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget-1];
        if (posX >= pGadget->xpos && posX < pGadget->xpos+pGadget->width &&
            posY >= pGadget->ypos && posY < pGadget->ypos+pGadget->height) {
            if (pGadgetIndex) *pGadgetIndex = iGadget-1;
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}


ta_result ta_gui_hold_gadget(ta_gui* pGUI, taUInt32 gadgetIndex, taUInt32 mouseButton)
{
    if (pGUI == NULL || gadgetIndex >= pGUI->gadgetCount) return TA_INVALID_ARGS;
    if (pGUI->heldGadgetIndex != (taUInt32)-1) {
        return TA_ERROR;    // Another gadget is already being held.
    }

    pGUI->pGadgets[gadgetIndex].isHeld = TA_TRUE;
    pGUI->pGadgets[gadgetIndex].heldMB = mouseButton;
    pGUI->heldGadgetIndex = gadgetIndex;

    return TA_SUCCESS;
}

ta_result ta_gui_release_hold(ta_gui* pGUI, taUInt32 gadgetIndex)
{
    if (pGUI == NULL || gadgetIndex >= pGUI->gadgetCount) return TA_INVALID_ARGS;
    pGUI->pGadgets[gadgetIndex].isHeld = TA_FALSE;
    pGUI->pGadgets[gadgetIndex].heldMB = 0;
    pGUI->heldGadgetIndex = (taUInt32)-1;
    
    return TA_SUCCESS;
}

taBool32 ta_gui_get_held_gadget(ta_gui* pGUI, taUInt32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (taUInt32)-1;
    if (pGUI == NULL) return TA_FALSE;

    if (pGadgetIndex) *pGadgetIndex = pGUI->heldGadgetIndex;
    return pGUI->heldGadgetIndex != (taUInt32)-1;
}

taBool32 ta_gui_find_gadget_by_name(ta_gui* pGUI, const char* name, taUInt32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (taUInt32)-1;
    if (pGUI == NULL) return TA_FALSE;

    for (taUInt32 iGadget = 0; iGadget < pGUI->gadgetCount; ++iGadget) {
        if (_stricmp(pGUI->pGadgets[iGadget].name, name) == 0) {
            if (pGadgetIndex) *pGadgetIndex = iGadget;
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}

taBool32 ta_gui_get_focused_gadget(ta_gui* pGUI, taUInt32* pGadgetIndex)
{
    if (pGadgetIndex) *pGadgetIndex = (taUInt32)-1;
    if (pGUI == NULL) return TA_FALSE;

    if (pGadgetIndex) *pGadgetIndex = pGUI->focusedGadgetIndex;
    return pGUI->focusedGadgetIndex != (taUInt32)-1;
}

taBool32 ta_gui__can_gadget_receive_focus(ta_gui_gadget* pGadget)
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
    
    taBool32 wasAnythingFocusedBeforehand = pGUI->focusedGadgetIndex != (taUInt32)-1; // <-- Only used to avoid infinite recursion.

    // We just keep looping over each gadget until we find one that can receive focus. If we reached the end we just
    // loop back to the start and try again, making sure we don't get stuck in an infinite recursion loop.
    for (taUInt32 iGadget = (wasAnythingFocusedBeforehand) ? pGUI->focusedGadgetIndex+1 : 1; iGadget < pGUI->gadgetCount; ++iGadget) { // <-- Don't include the root gadget.
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget];
        if (ta_gui__can_gadget_receive_focus(pGadget)) {
            pGUI->focusedGadgetIndex = iGadget;
            return;
        }
    }

    // If we get here it means we weren't able to find a new gadget to focus. In this case we just loop back to the start and try again.
    if (wasAnythingFocusedBeforehand) {
        pGUI->focusedGadgetIndex = (taUInt32)-1;
        ta_gui_focus_next_gadget(pGUI);
    }
}

void ta_gui_focus_prev_gadget(ta_gui* pGUI)
{
    if (pGUI == NULL || pGUI->gadgetCount == 0) return;
    
    // Everything works the same as ta_gui_focus_next_gadget(), only in reverse.
    taBool32 wasAnythingFocusedBeforehand = pGUI->focusedGadgetIndex != (taUInt32)-1; // <-- Only used to avoid infinite recursion.

    for (taUInt32 iGadget = (wasAnythingFocusedBeforehand) ? pGUI->focusedGadgetIndex : pGUI->gadgetCount; iGadget > 1; --iGadget) {   // <-- Don't include the root gadget.
        ta_gui_gadget* pGadget = &pGUI->pGadgets[iGadget-1];
        if (ta_gui__can_gadget_receive_focus(pGadget)) {
            pGUI->focusedGadgetIndex = iGadget-1;
            return;
        }
    }

    if (wasAnythingFocusedBeforehand) {
        pGUI->focusedGadgetIndex = (taUInt32)-1;
        ta_gui_focus_prev_gadget(pGUI);
    }
}

const char* ta_gui_get_button_text(ta_gui_gadget* pGadget, taUInt32 stage)
{
    if (pGadget == NULL) return NULL;
    if (stage == 0) {
        return pGadget->button.text;
    }

    if (pGadget->id != TA_GUI_GADGET_TYPE_BUTTON || (taInt32)stage >= pGadget->button.stages) {
        return NULL;
    }

    int i = 0;
    const char* text = pGadget->button.text;
    for (;;) {
        char c = *text++;
        if (c == '\0') {
            i += 1;
            if (i == stage) {
                return text;
            }
        }
    }

    return NULL;
}

ta_result ta_gui_set_listbox_items(ta_gui_gadget* pGadget, const char** pItems, taUInt32 count)
{
    if (pGadget == NULL || pGadget->id != TA_GUI_GADGET_TYPE_LISTBOX) return TA_INVALID_ARGS;
    
    if (pGadget->listbox.pItems != NULL) {
        free(pGadget->listbox.pItems);
        pGadget->listbox.itemCount = 0;
    }

    // Make a copy of each item.
    size_t payloadSize = 0;
    for (taUInt32 i = 0; i < count; ++i) {
        payloadSize += sizeof(*pGadget->listbox.pItems);
        payloadSize += ta_strlen_or_zero(pItems[i]) + 1;    // +1 for null terminator.
    }

    pGadget->listbox.pItems = (char**)malloc(payloadSize);
    if (pGadget->listbox.pItems == NULL) {
        return TA_OUT_OF_MEMORY;
    }

    // Payload format: [ptr0, ptr1, ... ptrN][str0, str1, ... strN];
    char* pDstStr = (char*)pGadget->listbox.pItems + (sizeof(*pGadget->listbox.pItems) * count);
    for (taUInt32 i = 0; i < count; ++i) {
        const char* pSrcStr = pItems[i];
        if (pSrcStr == NULL) pSrcStr = "";

        size_t srcLen = ta_strlen_or_zero(pItems[i]) + 1;
        memcpy(pDstStr, pSrcStr, srcLen);

        pGadget->listbox.pItems[i] = pDstStr;
        pDstStr += srcLen;
    }

    pGadget->listbox.itemCount = count;
    return TA_SUCCESS;
}

const char* ta_gui_get_listbox_item(ta_gui_gadget* pGadget, taUInt32 index)
{
    if (pGadget == NULL || pGadget->id != TA_GUI_GADGET_TYPE_LISTBOX || index >= pGadget->listbox.itemCount) return NULL;
    return pGadget->listbox.pItems[index];
}



// Common GUI
// ==========
ta_result ta_common_gui__create_texture_atlas(taEngineContext* pEngine, ta_common_gui* pCommonGUI, ta_texture_packer* pPacker, ta_texture** ppTexture)
{
    assert(pEngine != NULL);
    assert(pCommonGUI != NULL);
    assert(pPacker != NULL);
    assert(ppTexture != NULL);
    
    *ppTexture = ta_create_texture(pEngine->pGraphics, pPacker->width, pPacker->height, 1, pPacker->pImageData);
    if (*ppTexture == NULL) {
        return TA_FAILED_TO_CREATE_RESOURCE;
    }

    return TA_SUCCESS;
}

ta_result ta_common_gui_load(taEngineContext* pEngine, ta_common_gui* pCommonGUI)
{
    if (pCommonGUI == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pCommonGUI);

    pCommonGUI->pEngine = pEngine;

    ta_result result = ta_gaf_texture_group_init(pEngine, "anims/commongui.GAF", ta_color_mode_truecolor, &pCommonGUI->textureGroup);
    if (result != TA_SUCCESS) {
        return result;
    }

    taUInt32 iSequence;
    if (ta_gaf_texture_group_find_sequence_by_name(&pCommonGUI->textureGroup, "BUTTONS0", &iSequence)) {
        ta_gaf_texture_group_sequence* pSequence = pCommonGUI->textureGroup.pSequences + iSequence;

        pCommonGUI->buttons[0].frameIndex = pSequence->firstFrameIndex + 4;
        pCommonGUI->buttons[1].frameIndex = pSequence->firstFrameIndex + 8;
        pCommonGUI->buttons[2].frameIndex = pSequence->firstFrameIndex + 12;
        pCommonGUI->buttons[3].frameIndex = pSequence->firstFrameIndex + 16;
        pCommonGUI->buttons[4].frameIndex = pSequence->firstFrameIndex + 20;
        pCommonGUI->buttons[5].frameIndex = pSequence->firstFrameIndex + 24;
    }

    if (ta_gaf_texture_group_find_sequence_by_name(&pCommonGUI->textureGroup, "SLIDERS", &iSequence)) {
        taUInt32 firstFrameIndex = pCommonGUI->textureGroup.pSequences[iSequence].firstFrameIndex;
        pCommonGUI->scrollbar.arrowUpFrameIndex           = firstFrameIndex + 6;
        pCommonGUI->scrollbar.arrowUpPressedFrameIndex    = firstFrameIndex + 7;
        pCommonGUI->scrollbar.arrowDownFrameIndex         = firstFrameIndex + 8;
        pCommonGUI->scrollbar.arrowDownPressedFrameIndex  = firstFrameIndex + 9;
        pCommonGUI->scrollbar.arrowLeftFrameIndex         = firstFrameIndex + 16;
        pCommonGUI->scrollbar.arrowLeftPressedFrameIndex  = firstFrameIndex + 17;
        pCommonGUI->scrollbar.arrowRightFrameIndex        = firstFrameIndex + 18;
        pCommonGUI->scrollbar.arrowRightPressedFrameIndex = firstFrameIndex + 19;

        pCommonGUI->scrollbar.trackTopFrameIndex          = firstFrameIndex + 0;
        pCommonGUI->scrollbar.trackBottomFrameIndex       = firstFrameIndex + 2;
        pCommonGUI->scrollbar.trackMidVertFrameIndex      = firstFrameIndex + 1;
        pCommonGUI->scrollbar.trackLeftFrameIndex         = firstFrameIndex + 10;
        pCommonGUI->scrollbar.trackRightFrameIndex        = firstFrameIndex + 12;
        pCommonGUI->scrollbar.trackMidHorzFrameIndex      = firstFrameIndex + 11;

        pCommonGUI->scrollbar.thumbVertFrameIndex         = firstFrameIndex + 4;
        pCommonGUI->scrollbar.thumbHorzFrameIndex         = firstFrameIndex + 14;
        pCommonGUI->scrollbar.thumbCapTopFrameIndex       = firstFrameIndex + 3;
        pCommonGUI->scrollbar.thumbCapBotFrameIndex       = firstFrameIndex + 5;
    }

    return TA_SUCCESS;
}

ta_result ta_common_gui_unload(ta_common_gui* pCommonGUI)
{
    if (pCommonGUI == NULL) return TA_INVALID_ARGS;

    ta_gaf_texture_group_uninit(&pCommonGUI->textureGroup);
    return TA_SUCCESS;
}

ta_result ta_common_gui_get_button_frame(ta_common_gui* pCommonGUI, taUInt32 width, taUInt32 height, taUInt32* pFrameIndex)
{
    if (pCommonGUI == NULL) return TA_INVALID_ARGS;

    // The selection logic might be able to be improved here...
    taInt32 diffXClosest = INT32_MAX;
    taInt32 diffYClosest = INT32_MAX;
    taUInt32 iClosestButton = (taUInt32)-1;
    for (taUInt32 i = 0; i < ta_countof(pCommonGUI->buttons); ++i) {
        taInt32 diffX = (taInt32)pCommonGUI->textureGroup.pFrames[pCommonGUI->buttons[i].frameIndex].sizeX - (taInt32)width;
        taInt32 diffY = (taInt32)pCommonGUI->textureGroup.pFrames[pCommonGUI->buttons[i].frameIndex].sizeY - (taInt32)height;
        if (diffX == 0 && diffY == 0) {
            if (pFrameIndex) *pFrameIndex = pCommonGUI->buttons[i].frameIndex;
            return TA_SUCCESS;
        }

        if (ta_abs((taInt64)diffXClosest+(taInt64)diffYClosest) > ta_abs((taInt64)diffX+(taInt64)diffY)) {
            diffXClosest = diffX;
            diffYClosest = diffY;
            iClosestButton = i;
        }
    }

    if (pFrameIndex) *pFrameIndex = pCommonGUI->buttons[iClosestButton].frameIndex;
    return TA_SUCCESS;
}

ta_result ta_common_gui_get_multistage_button_frame(ta_common_gui* pCommonGUI, taUInt32 stages, taUInt32* pFrameIndex)
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

    taUInt32 iSequence;
    if (!ta_gaf_texture_group_find_sequence_by_name(&pCommonGUI->textureGroup, sequenceName, &iSequence)) {
        return TA_RESOURCE_NOT_FOUND;
    }

    if (pFrameIndex) *pFrameIndex = pCommonGUI->textureGroup.pSequences[iSequence].firstFrameIndex;
    return TA_SUCCESS;
}