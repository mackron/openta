// Copyright (C) 2018 David Reid. See included LICENSE file.

// Notes on config files:
//
// - Comments are C style: // and /* ... */
// - Objects start with [MY_OBJECT]
// - The contents of an object are wrapped in { ... } pairs
// - Each key/value pair is terminated with a semi-colon

ta_config_obj* ta_allocate_config_object()
{
    ta_config_obj* pObj = calloc(1, sizeof(*pObj));
    if (pObj == NULL) {
        return NULL;
    }

    return pObj;
}

ta_bool32 ta_config_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

char* ta_config_first_non_whitespace(char* str)
{
    if (str == NULL) {
        return NULL;
    }

    while (str[0] != '\0' && !(str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != '\v' && str[0] != '\f' && str[0] != '\r')) {
        str += 1;
    }

    return str;
}

ta_bool32 ta_config_is_on_line_comment(const char* configString)
{
    return configString[0] == '/' && configString[1] == '/';
}

ta_bool32 ta_config_is_on_block_comment(const char* configString)
{
    return configString[0] == '/' && configString[1] == '*';
}

char* ta_config_seek_to_end_of_line_comment(char* configString)
{
    while (*configString != '\0')
    {
        if (*configString++ == '\n') {
            break;
        }
    }

    return configString;
}

char* ta_config_seek_to_end_of_block_comment(char* configString)
{
    while (*configString != '\0')
    {
        if (configString[0] == '*' && configString[1] == '/') {
            configString += 2;
            break;
        }

        configString += 1;
    }

    return configString;
}

char* ta_config_next_token(char* configString)
{
    while (*configString != '\0')
    {
        // Skip any whitespace.
        configString = ta_config_first_non_whitespace(configString);
        if (configString == NULL) {
            return NULL;
        }

        if (*configString == '\0') {
            return NULL;
        }

        // Skip comments.
        if (ta_config_is_on_line_comment(configString))
        {
            configString += 2;
            configString = ta_config_seek_to_end_of_line_comment(configString);
        }
        else if (ta_config_is_on_block_comment(configString))
        {
            configString += 2;
            configString = ta_config_seek_to_end_of_block_comment(configString);
        }
        else
        {
            // It's not whitespace nor a comment. Should be a token.
            return configString;
        }
    }

    return configString;
}

ta_config_var* ta_config_push_new_var(ta_config_obj* pParentObj, const char* name, const char* value)
{
    assert(pParentObj != NULL);
    assert(name != NULL);
    assert(value != NULL);

    unsigned int chunkSize = 16;
    if (pParentObj->bufferSize <= pParentObj->varCount)
    {
        // Not enough space in the buffer - reallocate.
        pParentObj->bufferSize += chunkSize;

        ta_config_var* pNewVars = realloc(pParentObj->pVars, pParentObj->bufferSize * sizeof(*pNewVars));
        if (pNewVars == NULL) {
            return NULL;    // Failed to allocate new buffer.
        }

        pParentObj->pVars = pNewVars;
    }


    ta_config_var* pVar = pParentObj->pVars + pParentObj->varCount;
    pVar->pObject = NULL;
    pVar->name = name;
    pVar->value = value;
    
    pParentObj->varCount += 1;
    return pVar;
}

ta_config_obj* ta_config_push_new_subobj(ta_config_obj* pParentObj, const char* name, const char* value)
{
    assert(pParentObj != NULL);
    assert(name != NULL);
    assert(value != NULL);

    // The value should always be an empty string.
    assert(value[0] == '\0');


    ta_config_obj* pSubObj = ta_allocate_config_object();
    if (pSubObj == NULL) {
        return NULL;
    }

    ta_config_var* pVar = ta_config_push_new_var(pParentObj, name, value);
    if (pVar == NULL) {
        return NULL;
    }

    pVar->pObject = pSubObj;
    return pSubObj;
}


char* ta_parse_config_object(char* configString, ta_config_obj* pObj)
{
    // This is the where the real meat of the parsing is done. It assumes the config string is sitting on the byte just
    // after the opening curly bracket of the object.

    while (configString = ta_config_next_token(configString))
    {
        if (*configString == '\0') {
            return NULL;            // Reached the end of the string. Technically an error because we were expecting a closing curly bracket.
        }
        if (*configString == '}') {
            configString += 1;
            return configString;    // We've either reached the end of the object definition or the string itself.
        }


        if (*configString == '[')
        {
            // Beginning a sub-object. A sub-object is a variable of the parent object.
            configString += 1;  // Skip past the opening "["

            char* nameBeg = configString;
            char* nameEnd = nameBeg;
            while (*nameEnd != ']') {
                if (*nameEnd == '\0') {
                    return NULL;   // Unexpected end of file.
                }
                if (ta_config_is_on_line_comment(nameEnd)) {
                    return NULL;    // Expecting closing ']' but found a comment instead.
                }
                if (ta_config_is_on_block_comment(nameEnd)) {
                    return NULL;    // Expecting closing ']' but found a comment instead.
                }
                nameEnd += 1;
            }

            // Null terminate the name.
            *nameEnd = '\0';


            // Expecting an opening curly bracket.
            configString = ta_config_next_token(nameEnd + 1);
            if (configString == NULL || *configString == '\0') {
                return NULL;    // Unexpected end of file.
            }

            if (*configString++ != '{') {
                return NULL;    // Expecting '{'.
            }


            // We are beginning a sub-object so we'll need to call this recursively.
            ta_config_obj* pSubObj = ta_config_push_new_subobj(pObj, nameBeg, nameEnd);     // <-- Set the value to "nameEnd" which simply makes it an empty string rather than NULL which is a bit safer.
            if (pSubObj == NULL) {
                return NULL;    // Failed to allocate the sub-object.
            }

            configString = ta_parse_config_object(configString, pSubObj);
            if (configString == NULL) {
                return NULL;
            }
        }
        else
        {
            // Beginning a key/value pair. Format <name>=<value>;
            if (*configString != '_' && !(*configString >= 'a' && *configString <= 'z') && !(*configString >= 'A' && *configString <= 'Z')) {
                return NULL;    // Unexpected token. Variables must begin with a character or underscore.
            }

            char* nameBeg = configString++;
            char* nameEnd = configString;
            while (*configString != '=') {
                if (*configString == '\0') {
                    return NULL;    // Unexpected end of file.
                }
                if (ta_config_is_on_line_comment(configString)) {
                    return NULL;
                }
                if (ta_config_is_on_block_comment(configString)) {
                    return NULL;
                }

                configString += 1;
                if (!ta_config_is_whitespace(*configString)) {
                    nameEnd = configString;
                }
            }

            if (*configString != '=') {
                return NULL;    // Expecting '='.
            }

            // Null terminate the name.
            *nameEnd = '\0';

            // Skip past the '='.
            configString += 1;

            char* valueBeg = configString;
            char* valueEnd = configString;
            while (*configString != ';') {
                if (*configString == '\0') {
                    return NULL;    // Unexpected end of file.
                }
                if (ta_config_is_on_line_comment(configString)) {
                    return NULL;
                }
                if (ta_config_is_on_block_comment(configString)) {
                    return NULL;
                }

                configString += 1;
                if (!ta_config_is_whitespace(*configString)) {
                    valueEnd = configString;
                }
            }

            if (*configString != ';') {
                return NULL;    // Expecting ';'.
            }

            // Null terminate the value.
            *valueEnd = '\0';

            // Skip past the ';'
            configString += 1;

            ta_config_var* pVar = ta_config_push_new_var(pObj, nameBeg, valueBeg);
            if (pVar == NULL) {
                return NULL;
            }
        }
    }

    return configString;
}

ta_config_obj* ta_parse_config_from_open_file(ta_file* pFile)
{
    assert(pFile != NULL);

    ta_config_obj* pConfig = ta_allocate_config_object();
    if (pConfig == NULL) {
        return NULL;
    }

    pConfig->_pFile = pFile;
    if (pConfig->_pFile == NULL) {
        free(pConfig);
        return NULL;
    }

    ta_parse_config_object(pConfig->_pFile->pFileData, pConfig);
    return pConfig;
}

ta_config_obj* ta_parse_config_from_specific_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath)
{
    if (pFS == NULL || fileRelativePath == NULL) {
        return NULL;
    }

    ta_file* pFile = ta_open_specific_file(pFS, archiveRelativePath, fileRelativePath, TA_OPEN_FILE_WITH_NULL_TERMINATOR);
    if (pFile == NULL) {
        return NULL;
    }

    return ta_parse_config_from_open_file(pFile);
}

ta_config_obj* ta_parse_config_from_file(ta_fs* pFS, const char* fileRelativePath)
{
    if (pFS == NULL || fileRelativePath == NULL) {
        return NULL;
    }

    ta_file* pFile = ta_open_file(pFS, fileRelativePath, TA_OPEN_FILE_WITH_NULL_TERMINATOR);
    if (pFile == NULL) {
        return NULL;
    }

    return ta_parse_config_from_open_file(pFile);
}

void ta_delete_config(ta_config_obj* pConfig)
{
    if (pConfig == NULL) {
        return;
    }

    if (pConfig->_pFile) {
        ta_close_file(pConfig->_pFile);
    }


    // Sub objects need to be deleted recursively.
    for (unsigned int iVar = 0; iVar < pConfig->varCount; ++iVar) {
        if (pConfig->pVars[iVar].pObject != NULL) {
            ta_delete_config(pConfig->pVars[iVar].pObject);
        }
    }

    free(pConfig->pVars);
    free(pConfig);
}


ta_config_var* ta_config_get_var(const ta_config_obj* pConfig, const char* varName)
{
    if (pConfig == NULL || varName == NULL) {
        return NULL;
    }

    drpath_iterator iSeg;
    if (drpath_first(varName, &iSeg)) {
        drpath_iterator iNextSeg = iSeg;
        if (drpath_next(&iNextSeg)) {
            char subobjName[256];
            drpath_copy_and_append_iterator(subobjName, sizeof(subobjName), "", iSeg);
            ta_config_obj* pSubObj = ta_config_get_subobj(pConfig, subobjName);
            if (pSubObj != NULL) {
                return ta_config_get_var(pSubObj, iNextSeg.path + iNextSeg.segment.offset);
            }
        } else {
            for (unsigned int i = 0; i < pConfig->varCount; ++i) {
                if (strcmp(pConfig->pVars[i].name, varName) == 0) { // <-- Should this be case insensitive?
                    return &pConfig->pVars[i];
                }
            }
        }
    }

    return NULL;
}

ta_config_obj* ta_config_get_subobj(const ta_config_obj* pConfig, const char* varName)
{
    ta_config_var* pVar = ta_config_get_var(pConfig, varName);
    if (pVar == NULL) {
        return NULL;
    }

    return pVar->pObject;
}

const char* ta_config_get_string(const ta_config_obj* pConfig, const char* varName)
{
    ta_config_var* pVar = ta_config_get_var(pConfig, varName);
    if (pVar == NULL) {
        return NULL;
    }

    return pVar->value;
}

int ta_config_get_int(const ta_config_obj* pConfig, const char* varName)
{
    const char* value = ta_config_get_string(pConfig, varName);
    if (value == NULL) {
        return 0;
    }

    return atoi(value);
}

float ta_config_get_float(const ta_config_obj* pConfig, const char* varName)
{
    const char* value = ta_config_get_string(pConfig, varName);
    if (value == NULL) {
        return 0;
    }

    return (float)atof(value);
}

ta_bool32 ta_config_get_bool(const ta_config_obj* pConfig, const char* varName)
{
    const char* value = ta_config_get_string(pConfig, varName);
    if (value == NULL) {
        return TA_FALSE;
    }

    if (_stricmp(value, "FALSE") == 0 || value[0] == '0') {
        return TA_FALSE;
    }

    return TA_TRUE;
}

ta_bool32 ta_config_is_subobj_by_index(const ta_config_obj* pConfig, ta_uint32 varIndex)
{
    if (pConfig == NULL || varIndex >= pConfig->varCount) {
        return TA_FALSE;
    }

    return ta_is_string_null_or_empty(pConfig->pVars[varIndex].value);
}
