// Copyright (C) 2016 David Reid. See included LICENSE file.

struct ta_config_var
{
    // The name of the variable.
    const char* name;

    // The value of the variable, as a string. If it's an object, this will be an empty string.
    const char* value;

    // A pointer to the sub-object, if applicable.
    ta_config_obj* pObject;
};

struct ta_config_obj
{
    // The number of varibles making up the object.
    unsigned int varCount;

    // The size of the buffer containing the variables. This is used for determining whether or not a new allocation needs to be done.
    unsigned int bufferSize;

    // The list of variables making up the object.
    ta_config_var* pVars;


    // Internal use only. The file used to load the config. This is only set for the root object.
    ta_file* _pFile;
};

// Parses a script.
//
// Configs are immutable after parsing.
ta_config_obj* ta_parse_config_from_specific_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath);
ta_config_obj* ta_parse_config_from_file(ta_fs* pFS, const char* fileRelativePath);

// Deletes the given config object.
void ta_delete_config(ta_config_obj* pConfig);


// Retrieves a pointer to a generic variable from the given config.
ta_config_var* ta_config_get_var(ta_config_obj* pConfig, const char* varName);

// Retrieves the value of the given config variable as a sub-object. Returns NULL if the variable does not exist.
ta_config_obj* ta_config_get_subobj(ta_config_obj* pConfig, const char* varName);

// Retrieves the value of the given config variable as an integer. Returns NULL if the variable does not exist.
const char* ta_config_get_string(ta_config_obj* pConfig, const char* varName);

// Retrieves the value of the given config variable as an integer. Returns 0 if the variable does not exist.
int ta_config_get_int(ta_config_obj* pConfig, const char* varName);

// Retrieves the value of the given config variable as a float. Returns 0.0f if the variable does not exist.
float ta_config_get_float(ta_config_obj* pConfig, const char* varName);

// Retrieves the value of the given config variable as a boolean. Returns false if the variable does not exist. If the
// variable _does_ exist, it will return false if the value is equal to "false" (case-insensitive) or "0".
ta_bool32 ta_config_get_bool(ta_config_obj* pConfig, const char* varName);


// Determines if the variable at the given index is a sub-object.
ta_bool32 ta_config_is_subobj_by_index(ta_config_obj* pConfig, ta_uint32 varIndex);