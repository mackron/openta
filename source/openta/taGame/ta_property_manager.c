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
    for (ta_uint32 i = 0; i < pProperties->count; ++i) {
        free(pProperties->pProperties[i].key);
    }

    free(pProperties->pProperties);
    return TA_SUCCESS;
}


static ta_result ta_property_manager_load_default_totala_settings(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // TODO: Implement me.
    return TA_SUCCESS;
}

static ta_result ta_property_manager_load_totala_settings__config(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // TODO: Implement me.
    return TA_ERROR;
}

#ifdef _WIN32
static ta_uint32 ta_registry_read_uint32(HKEY hKey, const char* name)
{
    DWORD value = 0;
    DWORD valueSize = sizeof(value);
    LONG result = RegQueryValueExA(hKey, name, 0, NULL, (LPBYTE)&value, &valueSize);
    if (result != ERROR_SUCCESS) {
        return 0;
    }

    return value;
}

static char* ta_registry_read_string(HKEY hKey, const char* name, char* value, size_t valueBufferSize)  // Return value is <value> for convenience (null on error).
{
    LONG result = RegQueryValueExA(hKey, name, 0, NULL, (LPBYTE)value, (LPDWORD)&valueBufferSize);
    if (result != ERROR_SUCCESS) {
        return NULL;
    }

    return value;
}

static ta_result ta_property_manager_load_totala_settings__registry(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    char strBuffer[256];

    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Cavedog Entertainment\\Total Annihilation", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return TA_ERROR;
    }

    ta_property_manager_set_int(pProperties, "totala.ackfx",                    (int)ta_registry_read_uint32(hKey, "ackfx"));
    ta_property_manager_set_int(pProperties, "totala.anti-alias",               (int)ta_registry_read_uint32(hKey, "Anti-Alias"));
    ta_property_manager_set_int(pProperties, "totala.buildfx",                  (int)ta_registry_read_uint32(hKey, "buildfx"));
    ta_property_manager_set_int(pProperties, "totala.cdlists",                  (int)ta_registry_read_uint32(hKey, "CDLISTS"));
    ta_property_manager_set_int(pProperties, "totala.cdmode",                   (int)ta_registry_read_uint32(hKey, "cdmode"));
    ta_property_manager_set_int(pProperties, "totala.cdshell",                  (int)ta_registry_read_uint32(hKey, "cdshell"));
    ta_property_manager_set_int(pProperties, "totala.clock",                    (int)ta_registry_read_uint32(hKey, "clock"));
    ta_property_manager_set_int(pProperties, "totala.damagebars",               (int)ta_registry_read_uint32(hKey, "damagebars"));
    ta_property_manager_set_int(pProperties, "totala.difficulty",               (int)ta_registry_read_uint32(hKey, "Difficulty"));
    ta_property_manager_set_int(pProperties, "totala.display-width",            (int)ta_registry_read_uint32(hKey, "DisplaymodeWidth"));
    ta_property_manager_set_int(pProperties, "totala.display-height",           (int)ta_registry_read_uint32(hKey, "DisplaymodeHeight"));
    ta_property_manager_set_int(pProperties, "totala.dithered-fog",             (int)ta_registry_read_uint32(hKey, "DitheredFog"));
    ta_property_manager_set_int(pProperties, "totala.feature-shadows",          (int)ta_registry_read_uint32(hKey, "FeatureShadows"));
    ta_property_manager_set_int(pProperties, "totala.fixed-locations",          (int)ta_registry_read_uint32(hKey, "FixedLocations"));
    ta_property_manager_set_int(pProperties, "totala.fxvol",                    (int)ta_registry_read_uint32(hKey, "fxvol"));
    ta_property_manager_set(    pProperties, "totala.game-name",                     ta_registry_read_string(hKey, "Game Name", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set_int(pProperties, "totala.game-speed",               (int)ta_registry_read_uint32(hKey, "gamespeed"));
    ta_property_manager_set_int(pProperties, "totala.gamma",                    (int)ta_registry_read_uint32(hKey, "Gamma"));
    ta_property_manager_set_int(pProperties, "totala.interface-type",           (int)ta_registry_read_uint32(hKey, "Interface Type"));
    ta_property_manager_set_int(pProperties, "totala.mixing-buffers",           (int)ta_registry_read_uint32(hKey, "MixingBuffers"));
    ta_property_manager_set_int(pProperties, "totala.mouse-speed",              (int)ta_registry_read_uint32(hKey, "mousespeed"));
    ta_property_manager_set_int(pProperties, "totala.multi-commander-death",    (int)ta_registry_read_uint32(hKey, "MultiCommanderDeath"));
    ta_property_manager_set_int(pProperties, "totala.multi-line-of-sight",      (int)ta_registry_read_uint32(hKey, "MultiLineOfSight"));
    ta_property_manager_set_int(pProperties, "totala.multi-los-type",           (int)ta_registry_read_uint32(hKey, "MultiLOSType"));
    ta_property_manager_set_int(pProperties, "totala.multi-mapping",            (int)ta_registry_read_uint32(hKey, "MultiMapping"));
    ta_property_manager_set_int(pProperties, "totala.music-mode",               (int)ta_registry_read_uint32(hKey, "musicmode"));
    ta_property_manager_set_int(pProperties, "totala.musicvol",                 (int)ta_registry_read_uint32(hKey, "musicvol"));
    ta_property_manager_set(    pProperties, "totala.nickname",                      ta_registry_read_string(hKey, "Nickname", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set(    pProperties, "totala.password",                      ta_registry_read_string(hKey, "Password", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set_int(pProperties, "totala.play-movie",               (int)ta_registry_read_uint32(hKey, "PlayMovie"));
    ta_property_manager_set_int(pProperties, "totala.restore-volume",           (int)ta_registry_read_uint32(hKey, "RestoreVolume"));
    ta_property_manager_set_int(pProperties, "totala.screen-chat",              (int)ta_registry_read_uint32(hKey, "screenchat"));
    ta_property_manager_set_int(pProperties, "totala.scroll-speed",             (int)ta_registry_read_uint32(hKey, "scrollspeed"));
    ta_property_manager_set_int(pProperties, "totala.shading",                  (int)ta_registry_read_uint32(hKey, "Shading"));
    ta_property_manager_set_int(pProperties, "totala.shadows",                  (int)ta_registry_read_uint32(hKey, "Shadows"));
    ta_property_manager_set_int(pProperties, "totala.side",                     (int)ta_registry_read_uint32(hKey, "side"));
    ta_property_manager_set_int(pProperties, "totala.single-commander-death",   (int)ta_registry_read_uint32(hKey, "SingleCommanderDeath"));
    ta_property_manager_set_int(pProperties, "totala.single-line-of-sight",     (int)ta_registry_read_uint32(hKey, "SingleLineOfSight"));
    ta_property_manager_set_int(pProperties, "totala.single-los-type",          (int)ta_registry_read_uint32(hKey, "SingleLOSType"));
    ta_property_manager_set_int(pProperties, "totala.single-mapping",           (int)ta_registry_read_uint32(hKey, "SingleMapping"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-commander-death", (int)ta_registry_read_uint32(hKey, "SkirmishCommanderDeath"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-difficulty",      (int)ta_registry_read_uint32(hKey, "SkirmishDifficulty"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-line-of-sight",   (int)ta_registry_read_uint32(hKey, "SkirmishLineOfSight"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-location",        (int)ta_registry_read_uint32(hKey, "SkirmishLocation"));
    ta_property_manager_set_int(pProperties, "totala.skirmish-los-type",        (int)ta_registry_read_uint32(hKey, "SkirmishLOSType"));
    ta_property_manager_set(    pProperties, "totala.skirmish-map",                  ta_registry_read_string(hKey, "SkirmishMap", strBuffer, sizeof(strBuffer)));
    ta_property_manager_set_int(pProperties, "totala.skirmish-mapping",         (int)ta_registry_read_uint32(hKey, "SkirmishMapping"));
    ta_property_manager_set_int(pProperties, "totala.sound-mode",               (int)ta_registry_read_uint32(hKey, "Sound Mode"));
    ta_property_manager_set_int(pProperties, "totala.speechfx",                 (int)ta_registry_read_uint32(hKey, "speechfx"));
    ta_property_manager_set_int(pProperties, "totala.switch-alt",               (int)ta_registry_read_uint32(hKey, "SwitchAlt"));
    ta_property_manager_set_int(pProperties, "totala.text-lines",               (int)ta_registry_read_uint32(hKey, "textlines"));
    ta_property_manager_set_int(pProperties, "totala.text-scroll",              (int)ta_registry_read_uint32(hKey, "textscroll"));
    ta_property_manager_set_int(pProperties, "totala.unit-chat",                (int)ta_registry_read_uint32(hKey, "unitchat"));
    ta_property_manager_set_int(pProperties, "totala.unit-chat-text",           (int)ta_registry_read_uint32(hKey, "unitchattext"));
    ta_property_manager_set_int(pProperties, "totala.vehicle-shadows",          (int)ta_registry_read_uint32(hKey, "VehicleShadows"));

    RegCloseKey(hKey);


    result = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Cavedog Entertainment\\Total Annihilation\\Skirmish", 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        return TA_ERROR;
    }

    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-ally-group", (int)ta_registry_read_uint32(hKey, "Player0AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-color",      (int)ta_registry_read_uint32(hKey, "Player0Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-controller", (int)ta_registry_read_uint32(hKey, "Player0Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-energy",     (int)ta_registry_read_uint32(hKey, "Player0Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-metal",      (int)ta_registry_read_uint32(hKey, "Player0Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player0-side",       (int)ta_registry_read_uint32(hKey, "Player0Side"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-ally-group", (int)ta_registry_read_uint32(hKey, "Player1AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-color",      (int)ta_registry_read_uint32(hKey, "Player1Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-controller", (int)ta_registry_read_uint32(hKey, "Player1Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-energy",     (int)ta_registry_read_uint32(hKey, "Player1Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-metal",      (int)ta_registry_read_uint32(hKey, "Player1Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player1-side",       (int)ta_registry_read_uint32(hKey, "Player1Side"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-ally-group", (int)ta_registry_read_uint32(hKey, "Player2AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-color",      (int)ta_registry_read_uint32(hKey, "Player2Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-controller", (int)ta_registry_read_uint32(hKey, "Player2Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-energy",     (int)ta_registry_read_uint32(hKey, "Player2Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-metal",      (int)ta_registry_read_uint32(hKey, "Player2Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player2-side",       (int)ta_registry_read_uint32(hKey, "Player2Side"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-ally-group", (int)ta_registry_read_uint32(hKey, "Player3AllyGroup"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-color",      (int)ta_registry_read_uint32(hKey, "Player3Color"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-controller", (int)ta_registry_read_uint32(hKey, "Player3Controller"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-energy",     (int)ta_registry_read_uint32(hKey, "Player3Energy"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-metal",      (int)ta_registry_read_uint32(hKey, "Player3Metal"));
    ta_property_manager_set_int(pProperties, "totala.skirmish.player3-side",       (int)ta_registry_read_uint32(hKey, "Player3Side"));

    RegCloseKey(hKey);

    return TA_SUCCESS;
}
#endif

static ta_result ta_property_manager_load_totala_settings(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // Load defaults first, and then overwrite them with the actual settings from the config file or registry.
    ta_result result = ta_property_manager_load_default_totala_settings(pProperties);
    if (result != TA_SUCCESS) {
        return result;
    }

    // Try loading the original Total Annihilation settings from a config file first. If that fails and we're running on Windows,
    // try the registry.
    result = ta_property_manager_load_totala_settings__config(pProperties);
    if (result != TA_SUCCESS) {
#ifdef _WIN32
        return ta_property_manager_load_totala_settings__registry(pProperties);
#else
        return result;
#endif
    }

    return TA_SUCCESS;
}

ta_result ta_property_manager_load_settings(ta_property_manager* pProperties)
{
    if (pProperties == NULL) return TA_INVALID_ARGS;

    // Original Total Annihilation settings from the registry.
    ta_result result = ta_property_manager_load_totala_settings(pProperties);
    if (result != TA_SUCCESS) {
        return result;
    }

    // Here is where OpenTA-specific properties would be loaded from openta.cfg.

    return TA_SUCCESS;
}


static ta_result ta_property_manager_save_totala_settings(ta_property_manager* pProperties)
{
    assert(pProperties != NULL);

    // Just save these to totala.cfg for now, but may want to consider writing to the registry
    // for compatibility with totala.exe later on.

    // TODO: Implement me.
    return TA_SUCCESS;
}

ta_result ta_property_manager_save_settings(ta_property_manager* pProperties)
{
    if (pProperties != NULL) return TA_INVALID_ARGS;

    // Save the original Total Annihilation settings first.
    ta_property_manager_save_totala_settings(pProperties);

    // Here is where OpenTA-specific properties should be saved to openta.cfg.

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
            ta_uint32 newCapacity = (pProperties->capacity == 0) ? 16 : pProperties->capacity*2;
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
        for (ta_uint32 i = 0; i < pProperties->count; ++i) {
            if (strcmp(pProperties->pProperties[i].key, key) == 0) {
                for (ta_uint32 j = i; j < pProperties->count-1; ++j) {
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

ta_result ta_property_manager_set_bool(ta_property_manager* pProperties, const char* key, dr_bool32 val)
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

    for (ta_uint32 i = 0; i < pProperties->count; ++i) {
        if (strcmp(pProperties->pProperties[i].key, key) == 0) {
            return pProperties->pProperties[i].val;
        }
    }

    // Property does not exist.
    return NULL;
}