// Public domain. See "unlicense" statement at the end of this file.

#define TA_FEATURE_CHUNK_SIZE   1024

static uint32_t ta_features_library__hash_string(const char* str)
{
    return hashlittle(str, strlen(str), 0);
}

#if 0
static ta_feature_category ta_parse_feature_category(const char* featureStr)
{
    if (_stricmp(featureStr, "heaps") == 0) {
        return ta_feature_category_heaps;
    }
    if (_stricmp(featureStr, "arm_corpses") == 0) {
        return ta_feature_category_arm_corpses;
    }
    if (_stricmp(featureStr, "core_corpses") == 0 || _stricmp(featureStr, "cor_corpses") == 0) {
        return ta_feature_category_core_corpses;
    }
    if (_stricmp(featureStr, "rocks") == 0) {
        return ta_feature_category_rocks;
    }
    if (_stricmp(featureStr, "shrubs") == 0) {
        return ta_feature_category_shrubs;
    }
    if (_stricmp(featureStr, "foliage") == 0) {
        return ta_feature_category_foliage;
    }
    if (_stricmp(featureStr, "trees") == 0) {
        return ta_feature_category_trees;
    }
    if (_stricmp(featureStr, "ruins") == 0 || _stricmp(featureStr, "ruin") == 0) {
        return ta_feature_category_ruins;
    }
    if (_stricmp(featureStr, "scars") == 0) {
        return ta_feature_category_scars;
    }
    if (_stricmp(featureStr, "metal") == 0) {
        return ta_feature_category_metal;
    }
    if (_stricmp(featureStr, "steamvents") == 0) {
        return ta_feature_category_steamvents;
    }
    if (_stricmp(featureStr, "spires") == 0) {
        return ta_feature_category_spires;
    }
    if (_stricmp(featureStr, "gasplants") == 0) {
        return ta_feature_category_gasplants;
    }
    if (_stricmp(featureStr, "anemones") == 0) {
        return ta_feature_category_anemones;
    }
    if (_stricmp(featureStr, "corals") == 0) {
        return ta_feature_category_corals;
    }
    if (_stricmp(featureStr, "aquacorals") == 0) {
        return ta_feature_category_aquacorals;
    }
    if (_stricmp(featureStr, "smudges") == 0) {
        return ta_feature_category_smudges;
    }
    if (_stricmp(featureStr, "tracks") == 0) {
        return ta_feature_category_tracks;
    }
    if (_stricmp(featureStr, "buildings") == 0 || _stricmp(featureStr, "building") == 0) {
        return ta_feature_category_buildings;
    }
    if (_stricmp(featureStr, "machines") == 0) {
        return ta_feature_category_machines;
    }
    if (_stricmp(featureStr, "trucks") == 0) {
        return ta_feature_category_trucks;
    }
    if (_stricmp(featureStr, "cars") == 0) {
        return ta_feature_category_cars;
    }
    if (_stricmp(featureStr, "barriers") == 0) {
        return ta_feature_category_barriers;
    }
    if (_stricmp(featureStr, "nodes") == 0) {
        return ta_feature_category_nodes;
    }
    if (_stricmp(featureStr, "dragonteeth") == 0) {
        return ta_feature_category_dragonteeth;
    }
    if (_stricmp(featureStr, "glyph") == 0) {
        return ta_feature_category_glyph;
    }
    if (_stricmp(featureStr, "plants") == 0) {
        return ta_feature_category_plants;
    }
    if (_stricmp(featureStr, "pipes") == 0) {
        return ta_feature_category_pipes;
    }
    if (_stricmp(featureStr, "holes") == 0) {
        return ta_feature_category_holes;
    }
    if (_stricmp(featureStr, "craters") == 0) {
        return ta_feature_category_craters;
    }
    if (_stricmp(featureStr, "kelp") == 0) {
        return ta_feature_category_kelp;
    }
    if (_stricmp(featureStr, "monuments") == 0) {
        return ta_feature_category_monuments;
    }

    printf("Unknown feature category: %s\n", featureStr);
    return ta_feature_category_unknown;
}
#endif

static bool ta_features_library__load_feature(ta_features_library* pLib, const char* featureName, ta_config_obj* pFeatureConfig)
{
    if (pLib == NULL || pFeatureConfig == NULL) {
        return false;
    }

    // Add the feature to the list.
    if (pLib->featuresCount == pLib->featuresBufferSize)
    {
        // Need to reallocate.
        uint32_t newBufferSize = pLib->featuresBufferSize + TA_FEATURE_CHUNK_SIZE;
        ta_feature_desc* pNewFeatures = realloc(pLib->pFeatures, newBufferSize * sizeof(*pLib->pFeatures));
        if (pNewFeatures == NULL) {
            return false;   // Failed to allocate memory.
        }

        pLib->featuresBufferSize = newBufferSize;
        pLib->pFeatures = pNewFeatures;
    }

    ta_feature_desc* pFeature = pLib->pFeatures + pLib->featuresCount;
    memset(pFeature, 0, sizeof(*pFeature));
    pFeature->hash = ta_features_library__hash_string(featureName);

    for (uint32_t iVar = 0; iVar < pFeatureConfig->varCount; ++iVar)
    {
        ta_config_var* pVar = pFeatureConfig->pVars + iVar;

        if (_stricmp(pVar->name, "description") == 0) {
            strncpy_s(pFeature->description, sizeof(pFeature->description), pVar->value, _TRUNCATE);
            continue;
        }

#if 0
        if (_stricmp(pVar->name, "category") == 0) {
            pFeature->category = ta_parse_feature_category(pVar->value);
            continue;
        }
#endif

        if (_stricmp(pVar->name, "filename") == 0) {
            strcpy_s(pFeature->filename, sizeof(pFeature->filename), pVar->value);
            continue;
        }
        if (_stricmp(pVar->name, "seqname") == 0) {
            strcpy_s(pFeature->seqname, sizeof(pFeature->seqname), pVar->value);
            continue;
        }
        if (_stricmp(pVar->name, "seqnameburn") == 0) {
            strcpy_s(pFeature->seqnameburn, sizeof(pFeature->seqnameburn), pVar->value);
            continue;
        }
        if (_stricmp(pVar->name, "seqnamedie") == 0) {
            strcpy_s(pFeature->seqnamedie, sizeof(pFeature->seqnamedie), pVar->value);
            continue;
        }
        if (_stricmp(pVar->name, "seqnamereclamate") == 0) {
            strcpy_s(pFeature->seqnamereclamate, sizeof(pFeature->seqnamereclamate), pVar->value);
            continue;
        }
        if (_stricmp(pVar->name, "seqnameshad") == 0) {
            strcpy_s(pFeature->seqnameshadow, sizeof(pFeature->seqnameshadow), pVar->value);
            continue;
        }
        if (_stricmp(pVar->name, "object") == 0) {
            strcpy_s(pFeature->object, sizeof(pFeature->object), pVar->value);
            continue;
        }


        // Booleans
        if (_stricmp(pVar->name, "animating") == 0) {
            pFeature->flags |= TA_FEATURE_ANIMATING;
            continue;
        }
        if (_stricmp(pVar->name, "animtrans") == 0) {
            pFeature->flags |= TA_FEATURE_ANIMTRANS;
            continue;
        }
        if (_stricmp(pVar->name, "autoreclaimable") == 0) {
            pFeature->flags |= TA_FEATURE_AUTORECLAIMABLE;
            continue;
        }
        if (_stricmp(pVar->name, "blocking") == 0) {
            pFeature->flags |= TA_FEATURE_BLOCKING;
            continue;
        }
        if (_stricmp(pVar->name, "flamable") == 0) {
            pFeature->flags |= TA_FEATURE_FLAMABLE;
            continue;
        }
        if (_stricmp(pVar->name, "geothermal") == 0) {
            pFeature->flags |= TA_FEATURE_GEOTHERMAL;
            continue;
        }
        if (_stricmp(pVar->name, "indestructible") == 0) {
            pFeature->flags |= TA_FEATURE_INDESTRUCTIBLE;
            continue;
        }
        if (_stricmp(pVar->name, "nodisplayinfo") == 0) {
            pFeature->flags |= TA_FEATURE_NODISPLAYINFO;
            continue;
        }
        if (_stricmp(pVar->name, "permanent") == 0) {
            pFeature->flags |= TA_FEATURE_PERMANENT;
            continue;
        }
        if (_stricmp(pVar->name, "reclaimable") == 0) {
            pFeature->flags |= TA_FEATURE_RECLAIMABLE;
            continue;
        }
        if (_stricmp(pVar->name, "shadtrans") == 0) {
            pFeature->flags |= TA_FEATURE_SHADOWTRANSPARENT;
            continue;
        }
    }


    // Add the feature to the end of the list.
    pLib->featuresCount += 1;

    // When a new feature is added, the library is no longer in an optimized state.
    pLib->isOptimized = false;

    return true;
}

static bool ta_features_library__load_features_from_config(ta_features_library* pLib, ta_config_obj* pConfig)
{
    if (pLib == NULL) {
        return false;
    }

    bool result = true;
    for (unsigned int iVar = 0; iVar < pConfig->varCount; ++iVar) {
        if (!ta_features_library__load_feature(pLib, pConfig->pVars[iVar].name, pConfig->pVars[iVar].pObject)) {
            result = false;
            break;
        }
    }

    return result;
}

static bool ta_features_library__load_and_parse_script(ta_features_library* pLib, ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath)
{
    assert(pLib != NULL);
    assert(pFS != NULL);
    assert(fileRelativePath != NULL);

    ta_config_obj* pConfig = ta_parse_config_from_file(pFS, archiveRelativePath, fileRelativePath);
    if (pConfig == NULL) {
        return false;
    }

    bool result = ta_features_library__load_features_from_config(pLib, pConfig);
    //printf("FILE: %s/%s\n", archiveRelativePath, fileRelativePath);

    ta_delete_config(pConfig);
    return result;
}


static void ta_features_library__optimize(ta_features_library* pLib)
{
    assert(pLib != NULL);

    // TODO: Sort by hash; optimize buffer, set pLib->isOptimized to true.
    (void)pLib;
}


static ta_feature_desc* ta_features_library__find_by_hash(ta_features_library* pLib, uint32_t hash)
{
    assert(pLib != NULL);

    if (pLib->isOptimized)
    {
        // Optimized binary search.
        // TODO: Implement.
    }
    else
    {
        // Linear search.
        for (uint32_t iFeature = 0; iFeature < pLib->featuresCount; ++iFeature) {
            if (pLib->pFeatures[iFeature].hash == hash) {
                return pLib->pFeatures + iFeature;
            }
        }
    }

    return NULL;
}



ta_features_library* ta_create_features_library(ta_fs* pFS)
{
    if (pFS == NULL) {
        return NULL;
    }

    ta_features_library* pLib = malloc(sizeof(*pLib));
    if (pLib == NULL) {
        return NULL;
    }

    pLib->featuresCount = 0;
    pLib->featuresBufferSize = TA_FEATURE_CHUNK_SIZE;
    pLib->pFeatures = malloc(pLib->featuresBufferSize * sizeof(*pLib->pFeatures));
    if (pLib->pFeatures == NULL) {
        free(pLib);
        return NULL;
    }

    ta_fs_iterator* pIter = ta_fs_begin(pFS, "features", true);     // <-- "true" means to search recursively.
    while (ta_fs_next(pIter)) {
        if (!pIter->fileInfo.isDirectory && drpath_extension_equal(pIter->fileInfo.relativePath, "tdf")) {
            ta_features_library__load_and_parse_script(pLib, pFS, pIter->fileInfo.archiveRelativePath, pIter->fileInfo.relativePath);
        }
    }
    ta_fs_end(pIter);


    // The features library is immutable which means we can optimize a few things for efficiency.
    ta_features_library__optimize(pLib);

    return pLib;
}

void ta_delete_features_library(ta_features_library* pLib)
{
    if (pLib == NULL) {
        return;
    }

    free(pLib->pFeatures);
    free(pLib);
}


ta_feature_desc* ta_find_feature_desc(ta_features_library* pLib, const char* featureName)
{
    return ta_features_library__find_by_hash(pLib, ta_features_library__hash_string(featureName));
}
