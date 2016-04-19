// Public domain. See "unlicense" statement at the end of this file.

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

bool ta_config_is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

bool ta_config_is_on_line_comment(const char* configString)
{
    return configString[0] == '/' && configString[1] == '/';
}

bool ta_config_is_on_block_comment(const char* configString)
{
    return configString[0] == '/' && configString[1] == '*';
}

const char* ta_config_seek_to_end_of_line_comment(const char* configString)
{
    while (*configString != '\0')
    {
        if (*configString++ == '\n') {
            break;
        }
    }

    return configString;
}

const char* ta_config_seek_to_end_of_block_comment(const char* configString)
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

const char* ta_config_next_token(const char* configString)
{
    while (*configString != '\0')
    {
        // Skip any whitespace.
        configString = dr_first_non_whitespace(configString);
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

ta_config_var* ta_config_push_new_var(ta_config_obj* pParentObj, const char* nameBeg, const char* nameEnd, const char* valueBeg, const char* valueEnd)
{
    assert(pParentObj != NULL);
    assert(nameBeg != NULL);
    assert(nameEnd != NULL);

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
    strncpy_s(pVar->name, sizeof(pVar->name), nameBeg, nameEnd - nameBeg);

    if (valueBeg) {
        strncpy_s(pVar->value, sizeof(pVar->value), valueBeg, valueEnd - valueBeg);
    } else {
        pVar->value[0] = '\0';
    }
    
    pParentObj->varCount += 1;
    return pVar;
}

ta_config_obj* ta_config_push_new_subobj(ta_config_obj* pParentObj, const char* nameBeg, const char* nameEnd)
{
    assert(pParentObj != NULL);
    assert(nameBeg != NULL);
    assert(nameEnd != NULL);

    ta_config_obj* pSubObj = ta_allocate_config_object();
    if (pSubObj == NULL) {
        return NULL;
    }

    ta_config_var* pVar = ta_config_push_new_var(pParentObj, nameBeg, nameEnd, NULL, NULL);
    if (pVar == NULL) {
        return NULL;
    }

    pVar->pObject = pSubObj;
    return pSubObj;
}


const char* ta_parse_config_object(const char* configString, ta_config_obj* pObj)
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

            const char* nameBeg = configString;
            const char* nameEnd = nameBeg;
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


            // Expecting an opening curly bracket.
            configString = ta_config_next_token(nameEnd + 1);
            if (configString == NULL || *configString == '\0') {
                return NULL;    // Unexpected end of file.
            }

            if (*configString++ != '{') {
                return NULL;    // Expecting '{'.
            }


            // We are beginning a sub-object so we'll need to call this recursively.
            ta_config_obj* pSubObj = ta_config_push_new_subobj(pObj, nameBeg, nameEnd);
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

            const char* nameBeg = configString++;
            const char* nameEnd = configString;
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

            configString += 1;  // Skip past the '='.

            const char* valueBeg = configString++;
            const char* valueEnd = configString;
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

            configString += 1;  // Skip past the ';'

            ta_config_var* pVar = ta_config_push_new_var(pObj, nameBeg, nameEnd, valueBeg, valueEnd);
            if (pVar == NULL) {
                return NULL;
            }
        }
    }

    return configString;
}

ta_config_obj* ta_parse_config(const char* configString)
{
    if (configString == NULL) {
        return NULL;
    }

    ta_config_obj* pRootObj = ta_allocate_config_object();
    if (pRootObj == NULL) {
        return NULL;
    }

    ta_parse_config_object(configString, pRootObj);
    return pRootObj;
}

void ta_delete_config(ta_config_obj* pConfig)
{
    if (pConfig == NULL) {
        return;
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