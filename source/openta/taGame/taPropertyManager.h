// Copyright (C) 2018 David Reid. See included LICENSE file.

// TODO: This needs an almost complete re-implementation:
// - Add support for changing the value of existing keys.
// - Add support for non-string values: numbers, booleans, strings

typedef struct
{
    // The key and value is stored in the same memory allocation. The value is stored just after
    // the null terminator of the key.
    char* key;
    char* val;
} ta_property;

typedef struct
{
    // TODO: This can be improved. It's currently just a simple variable-length array, but the memory
    // management should be able to be improved a bit here.
    ta_uint32 count;
    ta_uint32 capacity;
    ta_property* pProperties;
} ta_property_manager;

ta_result ta_property_manager_init(ta_property_manager* pProperties);
ta_result ta_property_manager_uninit(ta_property_manager* pProperties);

// Loads game settings from either the registry (Windows) or a config file.
ta_result ta_property_manager_load_settings(ta_property_manager* pProperties);

// Saves game settings to either the registry (Windows) or a config file.
ta_result ta_property_manager_save_settings(ta_property_manager* pProperties);

ta_result ta_property_manager_set(ta_property_manager* pProperties, const char* key, const char* val);
ta_result ta_property_manager_set_int(ta_property_manager* pProperties, const char* key, int val);
ta_result ta_property_manager_set_bool(ta_property_manager* pProperties, const char* key, dr_bool32 val);
ta_result ta_property_manager_unset(ta_property_manager* pProperties, const char* key);

const char* ta_property_manager_get(ta_property_manager* pProperties, const char* key);