// Copyright (C) 2018 David Reid. See included LICENSE file.

ta_result ta_property_manager_init(ta_property_manager* pProperties)
{
    if (pProperties == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pProperties);

    return TA_SUCCESS;
}

ta_result ta_property_manager_uninit(ta_property_manager* pProperties)
{
    if (pProperties == NULL) return TA_INVALID_ARGS;

    // Only the keys for each property is frees because the value is part of the same allocation.
    for (taUInt32 i = 0; i < pProperties->count; ++i) {
        free(pProperties->pProperties[i].key);
    }

    free(pProperties->pProperties);
    return TA_SUCCESS;
}


ta_result ta_property_manager_set(ta_property_manager* pProperties, const char* key, const char* val)
{
    if (pProperties == NULL || key == NULL) return TA_INVALID_ARGS;

    if (val != NULL) {
        size_t keyLen = strlen(key);
        size_t valLen = strlen(val);

        ta_property prop;
        prop.key = (char*)malloc(keyLen+1 + valLen+1); if (prop.key == NULL) return TA_OUT_OF_MEMORY;
        dr_strcpy_s(prop.key, keyLen+1, key);

        prop.val = prop.key + keyLen+1;
        dr_strcpy_s(prop.val, valLen+1, val);

        if (pProperties->capacity == pProperties->count) {
            taUInt32 newCapacity = (pProperties->capacity == 0) ? 16 : pProperties->capacity*2;
            ta_property* pNewProperties = (ta_property*)realloc(pProperties->pProperties, newCapacity * sizeof(ta_property));
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

ta_result ta_property_manager_set_int(ta_property_manager* pProperties, const char* key, int val)
{
    char valStr[16];
    if (_itoa_s(val, valStr, sizeof(valStr), 10) != 0) {
        return TA_ERROR;
    }

    return ta_property_manager_set(pProperties, key, valStr);
}

ta_result ta_property_manager_set_bool(ta_property_manager* pProperties, const char* key, taBool32 val)
{
    return ta_property_manager_set(pProperties, key, (val) ? "true" : "false");
}

ta_result ta_property_manager_unset(ta_property_manager* pProperties, const char* key)
{
    return ta_property_manager_set(pProperties, key, NULL);
}

const char* ta_property_manager_get(ta_property_manager* pProperties, const char* key)
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

const char* ta_property_manager_getv(ta_property_manager* pProperties, const char* key, va_list args)
{
    const char* value = NULL;

    char* formattedKey = ta_make_stringv(key, args);
    if (formattedKey != NULL) {
        value = ta_property_manager_get(pProperties, formattedKey);
        ta_free_string(formattedKey);
    }

    return value;
}

const char* ta_property_manager_getf(ta_property_manager* pProperties, const char* key, ...)
{
    va_list args;
    va_start(args, key);

    const char* str = ta_property_manager_getv(pProperties, key, args);

    va_end(args);
    return str;
}