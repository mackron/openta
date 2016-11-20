// Copyright (C) 2016 David Reid. See included LICENSE file.

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

            // TODO: Parse string variables and accumulate their lengths so they can be dynamically allocated rather than fixed. This
            // will save memory _and_ make it more robust.
            //
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
                dr_strcpy_s(pGadget->name, sizeof(pGadget->name), ta_config_get_string(pCommonObj, "name"));
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
                dr_strcpy_s(pGadget->help, sizeof(pGadget->help), ta_config_get_string(pCommonObj, "help"));
            }

            switch (pGadget->id)
            {
                case TA_GUI_GADGET_TYPE_ROOT:
                {
                    pGadget->root.totalgadgets = ta_config_get_int(pGadgetObj, "totalgadgets");
                    dr_strcpy_s(pGadget->root.panel, sizeof(pGadget->root.panel), ta_config_get_string(pGadgetObj, "panel"));
                    dr_strcpy_s(pGadget->root.crdefault, sizeof(pGadget->root.crdefault), ta_config_get_string(pGadgetObj, "crdefault"));
                    dr_strcpy_s(pGadget->root.escdefault, sizeof(pGadget->root.escdefault), ta_config_get_string(pGadgetObj, "escdefault"));
                    dr_strcpy_s(pGadget->root.defaultfocus, sizeof(pGadget->root.defaultfocus), ta_config_get_string(pGadgetObj, "defaultfocus"));

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
                    dr_strcpy_s(pGadget->button.text, sizeof(pGadget->button.text), ta_config_get_string(pGadgetObj, "text"));
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
                    dr_strcpy_s(pGadget->label.text, sizeof(pGadget->label.text), ta_config_get_string(pGadgetObj, "text"));
                    dr_strcpy_s(pGadget->label.link, sizeof(pGadget->label.link), ta_config_get_string(pGadgetObj, "link"));
                } break;

                case TA_GUI_GADGET_TYPE_SURFACE:
                {
                    pGadget->surface.hotornot = ta_config_get_bool(pGadgetObj, "hotornot");
                } break;

                case TA_GUI_GADGET_TYPE_FONT:
                {
                    dr_strcpy_s(pGadget->font.filename, sizeof(pGadget->font.filename), ta_config_get_string(pGadgetObj, "filename"));
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

    pGUI->pGAF = ta_open_gaf(pGame->pFS, fileNameGAF);    // <-- This is allowed to be NULL.


    // I haven't yet found a way to determine the background image to use for GUIs, so for the moment we will hard code these.
    const char* propVal = ta_get_propertyf(pGame, "%s.BACKGROUND", fileName);



    return TA_SUCCESS;
}

ta_result ta_gui_unload(ta_gui* pGUI)
{
    if (pGUI == NULL) return TA_INVALID_ARGS;

    if (pGUI->pGAF) ta_close_gaf(pGUI->pGAF);
    free(pGUI->_pPayload);

    return TA_SUCCESS;
}