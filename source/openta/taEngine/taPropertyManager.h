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
} taProperty;

typedef struct
{
    // TODO: This can be improved. It's currently just a simple variable-length array, but the memory
    // management should be able to be improved a bit here.
    taUInt32 count;
    taUInt32 capacity;
    taProperty* pProperties;
} taPropertyManager;

taResult taPropertyManagerInit(taPropertyManager* pProperties);
taResult taPropertyManagerUninit(taPropertyManager* pProperties);

taResult taPropertyManagerSet(taPropertyManager* pProperties, const char* key, const char* val);
taResult taPropertyManagerSetInt(taPropertyManager* pProperties, const char* key, int val);
taResult taPropertyManagerSetBool(taPropertyManager* pProperties, const char* key, taBool32 val);
taResult taPropertyManagerUnset(taPropertyManager* pProperties, const char* key);

const char* taPropertyManagerGet(taPropertyManager* pProperties, const char* key);
const char* taPropertyManagerGetV(taPropertyManager* pProperties, const char* key, va_list args);
const char* taPropertyManagerGetF(taPropertyManager* pProperties, const char* key, ...);
