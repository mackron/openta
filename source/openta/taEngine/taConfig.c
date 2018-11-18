// Copyright (C) 2018 David Reid. See included LICENSE file.

// Notes on config files:
//
// - Comments are C style: // and /* ... */
// - Objects start with [MY_OBJECT]
// - The contents of an object are wrapped in { ... } pairs
// - Each key/value pair is terminated with a semi-colon

taConfigObj* taAllocateConfigObject()
{
    taConfigObj* pObj = calloc(1, sizeof(*pObj));
    if (pObj == NULL) {
        return NULL;
    }

    return pObj;
}

taBool32 taConfigIsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

char* taConfigFirstNonWhitespace(char* str)
{
    if (str == NULL) {
        return NULL;
    }

    while (str[0] != '\0' && !(str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != '\v' && str[0] != '\f' && str[0] != '\r')) {
        str += 1;
    }

    return str;
}

taBool32 taConfigIsOnLineComment(const char* configString)
{
    return configString[0] == '/' && configString[1] == '/';
}

taBool32 taConfigIsOnBlockComment(const char* configString)
{
    return configString[0] == '/' && configString[1] == '*';
}

char* taConfigSeekToEndOfLineComment(char* configString)
{
    while (*configString != '\0')
    {
        if (*configString++ == '\n') {
            break;
        }
    }

    return configString;
}

char* taConfigSeekToEndOfBlockComment(char* configString)
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

char* taConfigNextToken(char* configString)
{
    while (*configString != '\0')
    {
        // Skip any whitespace.
        configString = taConfigFirstNonWhitespace(configString);
        if (configString == NULL) {
            return NULL;
        }

        if (*configString == '\0') {
            return NULL;
        }

        // Skip comments.
        if (taConfigIsOnLineComment(configString))
        {
            configString += 2;
            configString = taConfigSeekToEndOfLineComment(configString);
        }
        else if (taConfigIsOnBlockComment(configString))
        {
            configString += 2;
            configString = taConfigSeekToEndOfBlockComment(configString);
        }
        else
        {
            // It's not whitespace nor a comment. Should be a token.
            return configString;
        }
    }

    return configString;
}

taConfigVar* taConfigPushNewVar(taConfigObj* pParentObj, const char* name, const char* value)
{
    assert(pParentObj != NULL);
    assert(name != NULL);
    assert(value != NULL);

    unsigned int chunkSize = 16;
    if (pParentObj->bufferSize <= pParentObj->varCount)
    {
        // Not enough space in the buffer - reallocate.
        pParentObj->bufferSize += chunkSize;

        taConfigVar* pNewVars = realloc(pParentObj->pVars, pParentObj->bufferSize * sizeof(*pNewVars));
        if (pNewVars == NULL) {
            return NULL;    // Failed to allocate new buffer.
        }

        pParentObj->pVars = pNewVars;
    }


    taConfigVar* pVar = pParentObj->pVars + pParentObj->varCount;
    pVar->pObject = NULL;
    pVar->name = name;
    pVar->value = value;
    
    pParentObj->varCount += 1;
    return pVar;
}

taConfigObj* taConfigPushNewSubObj(taConfigObj* pParentObj, const char* name, const char* value)
{
    assert(pParentObj != NULL);
    assert(name != NULL);
    assert(value != NULL);

    // The value should always be an empty string.
    assert(value[0] == '\0');


    taConfigObj* pSubObj = taAllocateConfigObject();
    if (pSubObj == NULL) {
        return NULL;
    }

    taConfigVar* pVar = taConfigPushNewVar(pParentObj, name, value);
    if (pVar == NULL) {
        return NULL;
    }

    pVar->pObject = pSubObj;
    return pSubObj;
}


char* taParseConfigObject(char* configString, taConfigObj* pObj)
{
    // This is the where the real meat of the parsing is done. It assumes the config string is sitting on the byte just
    // after the opening curly bracket of the object.

    while ((configString = taConfigNextToken(configString)) != NULL) {
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
                if (taConfigIsOnLineComment(nameEnd)) {
                    return NULL;    // Expecting closing ']' but found a comment instead.
                }
                if (taConfigIsOnBlockComment(nameEnd)) {
                    return NULL;    // Expecting closing ']' but found a comment instead.
                }
                nameEnd += 1;
            }

            // Null terminate the name.
            *nameEnd = '\0';


            // Expecting an opening curly bracket.
            configString = taConfigNextToken(nameEnd + 1);
            if (configString == NULL || *configString == '\0') {
                return NULL;    // Unexpected end of file.
            }

            if (*configString++ != '{') {
                return NULL;    // Expecting '{'.
            }


            // We are beginning a sub-object so we'll need to call this recursively.
            taConfigObj* pSubObj = taConfigPushNewSubObj(pObj, nameBeg, nameEnd);     // <-- Set the value to "nameEnd" which simply makes it an empty string rather than NULL which is a bit safer.
            if (pSubObj == NULL) {
                return NULL;    // Failed to allocate the sub-object.
            }

            configString = taParseConfigObject(configString, pSubObj);
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
                if (taConfigIsOnLineComment(configString)) {
                    return NULL;
                }
                if (taConfigIsOnBlockComment(configString)) {
                    return NULL;
                }

                configString += 1;
                if (!taConfigIsWhitespace(*configString)) {
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
                if (taConfigIsOnLineComment(configString)) {
                    return NULL;
                }
                if (taConfigIsOnBlockComment(configString)) {
                    return NULL;
                }

                configString += 1;
                if (!taConfigIsWhitespace(*configString)) {
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

            taConfigVar* pVar = taConfigPushNewVar(pObj, nameBeg, valueBeg);
            if (pVar == NULL) {
                return NULL;
            }
        }
    }

    return configString;
}

taConfigObj* taParseConfigFromOpenFile(taFile* pFile)
{
    assert(pFile != NULL);

    taConfigObj* pConfig = taAllocateConfigObject();
    if (pConfig == NULL) {
        return NULL;
    }

    pConfig->_pFile = pFile;
    if (pConfig->_pFile == NULL) {
        free(pConfig);
        return NULL;
    }

    taParseConfigObject(pConfig->_pFile->pFileData, pConfig);
    return pConfig;
}

taConfigObj* taParseConfigFromSpecificFile(taFS* pFS, const char* archiveRelativePath, const char* fileRelativePath)
{
    if (pFS == NULL || fileRelativePath == NULL) {
        return NULL;
    }

    taFile* pFile = taOpenSpecificFile(pFS, archiveRelativePath, fileRelativePath, TA_OPEN_FILE_WITH_NULL_TERMINATOR);
    if (pFile == NULL) {
        return NULL;
    }

    return taParseConfigFromOpenFile(pFile);
}

taConfigObj* taParseConfigFromFile(taFS* pFS, const char* fileRelativePath)
{
    if (pFS == NULL || fileRelativePath == NULL) {
        return NULL;
    }

    taFile* pFile = taOpenFile(pFS, fileRelativePath, TA_OPEN_FILE_WITH_NULL_TERMINATOR);
    if (pFile == NULL) {
        return NULL;
    }

    return taParseConfigFromOpenFile(pFile);
}

void taDeleteConfig(taConfigObj* pConfig)
{
    if (pConfig == NULL) {
        return;
    }

    if (pConfig->_pFile) {
        taCloseFile(pConfig->_pFile);
    }


    // Sub objects need to be deleted recursively.
    for (unsigned int iVar = 0; iVar < pConfig->varCount; ++iVar) {
        if (pConfig->pVars[iVar].pObject != NULL) {
            taDeleteConfig(pConfig->pVars[iVar].pObject);
        }
    }

    free(pConfig->pVars);
    free(pConfig);
}


taConfigVar* taConfigGetVar(const taConfigObj* pConfig, const char* varName)
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
            taConfigObj* pSubObj = taConfigGetSubObj(pConfig, subobjName);
            if (pSubObj != NULL) {
                return taConfigGetVar(pSubObj, iNextSeg.path + iNextSeg.segment.offset);
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

taConfigObj* taConfigGetSubObj(const taConfigObj* pConfig, const char* varName)
{
    taConfigVar* pVar = taConfigGetVar(pConfig, varName);
    if (pVar == NULL) {
        return NULL;
    }

    return pVar->pObject;
}

const char* taConfigGetString(const taConfigObj* pConfig, const char* varName)
{
    taConfigVar* pVar = taConfigGetVar(pConfig, varName);
    if (pVar == NULL) {
        return NULL;
    }

    return pVar->value;
}

int taConfigGetInt(const taConfigObj* pConfig, const char* varName)
{
    const char* value = taConfigGetString(pConfig, varName);
    if (value == NULL) {
        return 0;
    }

    return atoi(value);
}

float taConfigGetFloat(const taConfigObj* pConfig, const char* varName)
{
    const char* value = taConfigGetString(pConfig, varName);
    if (value == NULL) {
        return 0;
    }

    return (float)atof(value);
}

taBool32 taConfigGetBool(const taConfigObj* pConfig, const char* varName)
{
    const char* value = taConfigGetString(pConfig, varName);
    if (value == NULL) {
        return TA_FALSE;
    }

    if (_stricmp(value, "FALSE") == 0 || value[0] == '0') {
        return TA_FALSE;
    }

    return TA_TRUE;
}

taBool32 taConfigIsSubObjByIndex(const taConfigObj* pConfig, taUInt32 varIndex)
{
    if (pConfig == NULL || varIndex >= pConfig->varCount) {
        return TA_FALSE;
    }

    return taIsStringNullOrEmpty(pConfig->pVars[varIndex].value);
}
