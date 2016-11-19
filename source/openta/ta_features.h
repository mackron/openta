// Copyright (C) 2016 David Reid. See included LICENSE file.

#define TA_FEATURE_ANIMATING            0x0001
#define TA_FEATURE_ANIMTRANS            0x0002
#define TA_FEATURE_AUTORECLAIMABLE      0x0004
#define TA_FEATURE_BLOCKING             0x0008
#define TA_FEATURE_FLAMABLE             0x0010
#define TA_FEATURE_GEOTHERMAL           0x0020
#define TA_FEATURE_INDESTRUCTIBLE       0x0040
#define TA_FEATURE_NODISPLAYINFO        0x0080
#define TA_FEATURE_PERMANENT            0x0100
#define TA_FEATURE_RECLAIMABLE          0x0200
#define TA_FEATURE_SHADOWTRANSPARENT    0x0400

#if 0
// The different feature categories that the game actually uses.
typedef enum
{
    ta_feature_category_unknown,
    ta_feature_category_heaps,
    ta_feature_category_arm_corpses,
    ta_feature_category_core_corpses,
    ta_feature_category_rocks,
    ta_feature_category_shrubs,
    ta_feature_category_foliage,
    ta_feature_category_trees,
    ta_feature_category_ruins,
    ta_feature_category_scars,
    ta_feature_category_metal,
    ta_feature_category_steamvents,
    ta_feature_category_spires,
    ta_feature_category_gasplants,
    ta_feature_category_anemones,
    ta_feature_category_corals,
    ta_feature_category_aquacorals,
    ta_feature_category_smudges,
    ta_feature_category_tracks,
    ta_feature_category_buildings,
    ta_feature_category_machines,
    ta_feature_category_trucks,
    ta_feature_category_cars,
    ta_feature_category_barriers,
    ta_feature_category_nodes,
    ta_feature_category_dragonteeth,
    ta_feature_category_glyph,
    ta_feature_category_plants,
    ta_feature_category_pipes,
    ta_feature_category_holes,
    ta_feature_category_craters,
    ta_feature_category_kelp,
    ta_feature_category_monuments,
} ta_feature_category;
#endif

// Structure describing a feature. This is not a feature instantiation. The name of the feature is not
// currently stored for the sake of keeping this structure as small and simple as possible.
//
// Note that not every property is stored in this structure - only what we need.
struct ta_feature_desc
{
    // The name of the feature. This is used when searching for the feature.
    char name[64];

    // The description of the feature. This is shown at the bottom of the screen when the player places the mouse over the object.
    char description[64];

#if 0
    // The category. We're only using a set of predefined categories that we use which means this can be represented
    // with an enumerator.
    ta_feature_category category;
#endif

    // The width of the object, in 16x16 tiles.
    unsigned short footprintX;

    // The height of the object, in 16x16 tiles.
    unsigned short footprintY;

    // File .GAF file that contains the feature's graphics.
    char filename[64];

    // The name of the sequence within the GAF file of <filename> where the default graphic is located.
    char seqname[64];

    // Same as seqname, except for when the feature is burned.
    char seqnameburn[64];

    // The graphic used after the feature has been destroyed.
    char seqnamedie[64];

    // The graphic animation used when the feature is reclamated.
    char seqnamereclamate[64];

    // The graphic used to draw the feature's shadow.
    char seqnameshadow[64];

    // The 3DO object to use as the graphic for the feature. Blank for 2D features.
    char object[64];

    // Boolean properties
    //  TA_FEATURE_ANIMATING -> "animating"
    //  TA_FEATURE_ANIMTRANS -> "animtrans"
    //  TA_FEATURE_AUTORECLAIMABLE -> "autoreclaimable"
    //  TA_FEATURE_BLOCKING -> "blocking"
    //  TA_FEATURE_FLAMABLE -> "flamable"
    //  TA_FEATURE_GEOTHERMAL -> "geothermal"
    //  TA_FEATURE_INDESTRUCTIBLE -> "indestructible"
    //  TA_FEATURE_NODISPLAYINFO -> "nodisplayinfo"
    //  TA_FEATURE_PERMANENT -> "permanent"
    //  TA_FEATURE_RECLAIMABLE -> "reclaimable"
    //  TA_FEATURE_SHADOWTRANSPARENT -> "shadtrans"
    uint16_t flags;
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
    ta_bool32 isOptimized;
};

// Creates a features library by loading every TDF file in the "features" directory and all of it's sub-directories.
ta_features_library* ta_create_features_library(ta_fs* pFS);

// Deletes the given features library.
void ta_delete_features_library(ta_features_library* pLib);


// Finds a descriptor by name.
ta_feature_desc* ta_find_feature_desc(ta_features_library* pLib, const char* featureName);