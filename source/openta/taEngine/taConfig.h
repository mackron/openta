// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct taConfigObj taConfigObj;
typedef struct taConfigVar taConfigVar;

struct taConfigVar
{
    // The name of the variable.
    const char* name;

    // The value of the variable, as a string. If it's an object, this will be an empty string.
    const char* value;

    // A pointer to the sub-object, if applicable.
    taConfigObj* pObject;
};

struct taConfigObj
{
    // The number of varibles making up the object.
    unsigned int varCount;

    // The size of the buffer containing the variables. This is used for determining whether or not a new allocation needs to be done.
    unsigned int bufferSize;

    // The list of variables making up the object.
    taConfigVar* pVars;


    // Internal use only. The file used to load the config. This is only set for the root object.
    taFile* _pFile;
};

// Parses a script.
//
// Configs are immutable after parsing.
taConfigObj* taParseConfigFromSpecificFile(taFS* pFS, const char* archiveRelativePath, const char* fileRelativePath);
taConfigObj* taParseConfigFromFile(taFS* pFS, const char* fileRelativePath);

// Deletes the given config object.
void taDeleteConfig(taConfigObj* pConfig);


// Retrieves a pointer to a generic variable from the given config.
taConfigVar* taConfigGetVar(const taConfigObj* pConfig, const char* varName);

// Retrieves the value of the given config variable as a sub-object. Returns NULL if the variable does not exist.
taConfigObj* taConfigGetSubObj(const taConfigObj* pConfig, const char* varName);

// Retrieves the value of the given config variable as an integer. Returns NULL if the variable does not exist.
const char* taConfigGetString(const taConfigObj* pConfig, const char* varName);

// Retrieves the value of the given config variable as an integer. Returns 0 if the variable does not exist.
int taConfigGetInt(const taConfigObj* pConfig, const char* varName);

// Retrieves the value of the given config variable as a float. Returns 0.0f if the variable does not exist.
float taConfigGetFloat(const taConfigObj* pConfig, const char* varName);

// Retrieves the value of the given config variable as a boolean. Returns false if the variable does not exist. If the
// variable _does_ exist, it will return false if the value is equal to "false" (case-insensitive) or "0".
taBool32 taConfigGetBool(const taConfigObj* pConfig, const char* varName);


// Determines if the variable at the given index is a sub-object.
taBool32 taConfigIsSubObjByIndex(const taConfigObj* pConfig, taUInt32 varIndex);
