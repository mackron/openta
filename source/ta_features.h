// Public domain. See "unlicense" statement at the end of this file.

// Structure describing a feature. This is not a feature instantiation. The name of the feature is not
// currently stored for the sake of keeping this structure as small and simple as possible.
struct ta_feature_desc
{
    // The hash representing the feature descriptors name.
    uint32_t hash;  // <-- If collisions becomes an issue when using a 32-bit hash, consider switching to 64-bit.
};

// Structure containing the descriptors of every known feature. This structure is filled
// once during load time from TDF files contained in the "features" directory.
struct ta_features_library
{
    // The number of features in the library.
    uint32_t featuresCount;

    // The size of the buffer containing the features. This is used for determining whether or not the buffer needs to be reallocated.
    uint32_t featuresBufferSize;

    // The list of feature descriptors making up the library.
    ta_feature_desc* pFeatures;

    // Whether or not the library is optimized. If so, we can so a binary search for items.
    bool isOptimized;
};

// Creates a features library by loading every TDF file in the "features" directory and all of it's sub-directories.
ta_features_library* ta_create_features_library(ta_fs* pFS);

// Deletes the given features library.
void ta_delete_features_library(ta_features_library* pLib);


// Finds a descriptor by name.
ta_feature_desc* ta_find_feature_desc(ta_features_library* pLib, const char* featureName);