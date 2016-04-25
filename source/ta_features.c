// Public domain. See "unlicense" statement at the end of this file.

#define TA_FEATURE_CHUNK_SIZE   1024

static uint32_t ta_features_library__hash_string(const char* str)
{
    return hashlittle(str, strlen(str), 0);
}

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
    pFeature->hash = ta_features_library__hash_string(featureName);



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
