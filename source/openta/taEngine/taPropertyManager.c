// Copyright (C) 2018 David Reid. See included LICENSE file.

taResult taPropertyManagerInit(taPropertyManager* pProperties)
{
    if (pProperties == NULL) return TA_INVALID_ARGS;
    taZeroObject(pProperties);

    return TA_SUCCESS;
}

taResult taPropertyManagerUninit(taPropertyManager* pProperties)
{
    if (pProperties == NULL) return TA_INVALID_ARGS;

    // Only the keys for each property is frees because the value is part of the same allocation.
    for (taUInt32 i = 0; i < pProperties->count; ++i) {
        free(pProperties->pProperties[i].key);
    }

    free(pProperties->pProperties);
    return TA_SUCCESS;
}


taResult taPropertyManagerSet(taPropertyManager* pProperties, const char* key, const char* val)
{
    if (pProperties == NULL || key == NULL) return TA_INVALID_ARGS;

    if (val != NULL) {
        size_t keyLen = strlen(key);
        size_t valLen = strlen(val);

        taProperty prop;
        prop.key = (char*)malloc(keyLen+1 + valLen+1); if (prop.key == NULL) return TA_OUT_OF_MEMORY;
        dr_strcpy_s(prop.key, keyLen+1, key);

        prop.val = prop.key + keyLen+1;
        dr_strcpy_s(prop.val, valLen+1, val);

        if (pProperties->capacity == pProperties->count) {
            taUInt32 newCapacity = (pProperties->capacity == 0) ? 16 : pProperties->capacity*2;
            taProperty* pNewProperties = (taProperty*)realloc(pProperties->pProperties, newCapacity * sizeof(taProperty));
            if (pNewProperties == NULL) {
                return TA_OUT_OF_MEMORY;
            }

            pProperties->pProperties = pNewProperties;
            pProperties->capacity = newCapacity;
        }

        assert(pProperties->capacity > pProperties->count);
        pProperties->pProperties[pProperties->count] = prop;
        pProperties->count += 1;

        return TA_SUCCESS;
    } else {
        // The key is being unset so just clear it.
        for (taUInt32 i = 0; i < pProperties->count; ++i) {
            if (strcmp(pProperties->pProperties[i].key, key) == 0) {
                for (taUInt32 j = i; j < pProperties->count-1; ++j) {
                    pProperties->pProperties[j] = pProperties->pProperties[j+1];
                }

                pProperties->count -= 1;
                break;
            }
        }

        return TA_SUCCESS;
    }
}

taResult taPropertyManagerSetInt(taPropertyManager* pProperties, const char* key, int val)
{
    char valStr[16];
    if (_itoa_s(val, valStr, sizeof(valStr), 10) != 0) {
        return TA_ERROR;
    }

    return taPropertyManagerSet(pProperties, key, valStr);
}

taResult taPropertyManagerSetBool(taPropertyManager* pProperties, const char* key, taBool32 val)
{
    return taPropertyManagerSet(pProperties, key, (val) ? "true" : "false");
}

taResult taPropertyManagerUnset(taPropertyManager* pProperties, const char* key)
{
    return taPropertyManagerSet(pProperties, key, NULL);
}

const char* taPropertyManagerGet(taPropertyManager* pProperties, const char* key)
{
    if (pProperties == NULL || key == NULL) return NULL;

    for (taUInt32 i = 0; i < pProperties->count; ++i) {
        if (strcmp(pProperties->pProperties[i].key, key) == 0) {
            return pProperties->pProperties[i].val;
        }
    }

    // Property does not exist.
    return NULL;
}

const char* taPropertyManagerGetV(taPropertyManager* pProperties, const char* key, va_list args)
{
    const char* value = NULL;

    char* formattedKey = ta_make_stringv(key, args);
    if (formattedKey != NULL) {
        value = taPropertyManagerGet(pProperties, formattedKey);
        ta_free_string(formattedKey);
    }

    return value;
}

const char* taPropertyManagerGetF(taPropertyManager* pProperties, const char* key, ...)
{
    va_list args;
    va_start(args, key);

    const char* str = taPropertyManagerGetV(pProperties, key, args);

    va_end(args);
    return str;
}