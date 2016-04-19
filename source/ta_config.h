// Public domain. See "unlicense" statement at the end of this file.

struct ta_config_var
{
    // The name of the variable.
    char name[64];

    // The value of the variable, as a string. If it's an object, this will be an empty string.
    char value[64];

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
};

// Parses a script.
//
// Configs are immutable after parsing.
ta_config_obj* ta_parse_config(const char* configString);

// Deletes the given config object.
void ta_delete_config(ta_config_obj* pConfig);