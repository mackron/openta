// Copyright (C) 2018 David Reid. See included LICENSE file.

taBool32 taPathSegmentsEqual(const char* s0Path, const taPathSegment s0, const char* s1Path, const taPathSegment s1)
{
    if (s0Path == NULL || s1Path == NULL) {
        return TA_FALSE;
    }

    if (s0.length != s1.length) {
        return TA_FALSE;
    }

    return strncmp(s0Path + s0.offset, s1Path + s1.offset, s0.length) == 0;
}

taBool32 taPathFirst(const char* path, taPathIterator* i)
{
    if (i == 0) return TA_FALSE;
    i->path = path;
    i->segment.offset = 0;
    i->segment.length = 0;

    if (path == 0 || path[0] == '\0') {
        return TA_FALSE;
    }

    while (i->path[i->segment.length] != '\0' && (i->path[i->segment.length] != '/' && i->path[i->segment.length] != '\\')) {
        i->segment.length += 1;
    }

    return TA_TRUE;
}

taBool32 taPathLast(const char* path, taPathIterator* i)
{
    if (i == 0) return TA_FALSE;
    i->path = path;
    i->segment.offset = 0;
    i->segment.length = 0;

    if (path == 0 || path[0] == '\0') {
        return TA_FALSE;
    }

    i->path = path;
    i->segment.offset = strlen(path);
    i->segment.length = 0;

    return taPathPrev(i);
}

taBool32 taPathNext(taPathIterator* i)
{
    if (i == NULL || i->path == NULL) {
        return TA_FALSE;
    }

    i->segment.offset = i->segment.offset + i->segment.length;
    i->segment.length = 0;

    while (i->path[i->segment.offset] != '\0' && (i->path[i->segment.offset] == '/' || i->path[i->segment.offset] == '\\')) {
        i->segment.offset += 1;
    }

    if (i->path[i->segment.offset] == '\0') {
        return TA_FALSE;
    }


    while (i->path[i->segment.offset + i->segment.length] != '\0' && (i->path[i->segment.offset + i->segment.length] != '/' && i->path[i->segment.offset + i->segment.length] != '\\')) {
        i->segment.length += 1;
    }

    return TA_TRUE;
}

taBool32 taPathPrev(taPathIterator* i)
{
    if (i == NULL || i->path == NULL || i->segment.offset == 0) {
        return TA_FALSE;
    }

    i->segment.length = 0;

    do
    {
        i->segment.offset -= 1;
    } while (i->segment.offset > 0 && (i->path[i->segment.offset] == '/' || i->path[i->segment.offset] == '\\'));

    if (i->segment.offset == 0) {
        if (i->path[i->segment.offset] == '/' || i->path[i->segment.offset] == '\\') {
            i->segment.length = 0;
            return TA_TRUE;
        }

        return TA_FALSE;
    }


    size_t offsetEnd = i->segment.offset + 1;
    while (i->segment.offset > 0 && (i->path[i->segment.offset] != '/' && i->path[i->segment.offset] != '\\')) {
        i->segment.offset -= 1;
    }

    if (i->path[i->segment.offset] == '/' || i->path[i->segment.offset] == '\\') {
        i->segment.offset += 1;
    }


    i->segment.length = offsetEnd - i->segment.offset;

    return TA_TRUE;
}

taBool32 taPathAtEnd(taPathIterator i)
{
    return i.path == 0 || i.path[i.segment.offset] == '\0';
}

taBool32 taPathAtStart(taPathIterator i)
{
    return i.path != 0 && i.segment.offset == 0;
}

taBool32 taPathIteratorsEqual(const taPathIterator i0, const taPathIterator i1)
{
    return taPathSegmentsEqual(i0.path, i0.segment, i1.path, i1.segment);
}


taBool32 taPathIsRoot(const char* path)
{
    return taPathIsUnixStyleRoot(path) || taPathIsWin32StyleRoot(path);
}

taBool32 taPathIsRootSegment(const char* path, const taPathSegment segment)
{
    return taPathIsUnixStyleRootSegment(path, segment) || taPathIsWin32StyleRootSegment(path, segment);
}

taBool32 taPathIsUnixStyleRoot(const char* path)
{
    if (path == NULL) {
        return TA_FALSE;
    }

    if (path[0] == '/') {
        return TA_TRUE;
    }

    return TA_FALSE;
}

taBool32 taPathIsUnixStyleRootSegment(const char* path, const taPathSegment segment)
{
    if (path == NULL) {
        return TA_FALSE;
    }

    if (segment.offset == 0 && segment.length == 0) {
        return TA_TRUE;    // "/" style root.
    }

    return TA_FALSE;
}

taBool32 taPathIsWin32StyleRoot(const char* path)
{
    if (path == NULL) {
        return TA_FALSE;
    }

    if (((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':' && path[2] == '\0') {
        return TA_TRUE;
    }

    return TA_FALSE;
}

taBool32 taPathIsWin32StyleRootSegment(const char* path, const taPathSegment segment)
{
    if (path == NULL) {
        return TA_FALSE;
    }

    if (segment.offset == 0 && segment.length == 2) {
        if (((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':') {
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}




void taPathToForwardSlashes(char* path)
{
    if (path == NULL) {
        return;
    }

    while (path[0] != '\0') {
        if (path[0] == '\\') {
            path[0] = '/';
        }

        path += 1;
    }
}

void taPathToBackSlashes(char* path)
{
    if (path == NULL) {
        return;
    }

    while (path[0] != '\0') {
        if (path[0] == '/') {
            path[0] = '\\';
        }

        path += 1;
    }
}


taBool32 taPathIsDescendant(const char* descendantAbsolutePath, const char* parentAbsolutePath)
{
    taPathIterator iChild;
    if (!taPathFirst(descendantAbsolutePath, &iChild)) {
        return TA_FALSE;   // The descendant is an empty string which makes it impossible for it to be a descendant.
    }

    taPathIterator iParent;
    if (taPathFirst(parentAbsolutePath, &iParent)) {
        do {
            // If the segment is different, the paths are different and thus it is not a descendant.
            if (!taPathIteratorsEqual(iParent, iChild)) {
                return TA_FALSE;
            }

            if (!taPathNext(&iChild)) {
                return TA_FALSE;   // The descendant is shorter which means it's impossible for it to be a descendant.
            }
        } while (taPathNext(&iParent));
    }

    return TA_TRUE;
}

taBool32 taPathIsChild(const char* childAbsolutePath, const char* parentAbsolutePath)
{
    taPathIterator iChild;
    if (!taPathFirst(childAbsolutePath, &iChild)) {
        return TA_FALSE;   // The descendant is an empty string which makes it impossible for it to be a descendant.
    }

    taPathIterator iParent;
    if (taPathFirst(parentAbsolutePath, &iParent)) {
        do {
            // If the segment is different, the paths are different and thus it is not a descendant.
            if (!taPathIteratorsEqual(iParent, iChild)) {
                return TA_FALSE;
            }

            if (!taPathNext(&iChild)) {
                return TA_FALSE;   // The descendant is shorter which means it's impossible for it to be a descendant.
            }
        } while (taPathNext(&iParent));
    }

    // At this point we have finished iteration of the parent, which should be shorter one. We now do one more iterations of
    // the child to ensure it is indeed a direct child and not a deeper descendant.
    return !taPathNext(&iChild);
}


const char* taPathFileName(const char* path)
{
    if (path == NULL) {
        return NULL;
    }

    const char* fileName = path;

    // We just loop through the path until we find the last slash.
    while (path[0] != '\0') {
        if (path[0] == '/' || path[0] == '\\') {
            fileName = path;
        }

        path += 1;
    }

    // At this point the file name is sitting on a slash, so just move forward.
    while (fileName[0] != '\0' && (fileName[0] == '/' || fileName[0] == '\\')) {
        fileName += 1;
    }

    return fileName;
}

const char* taPathExtension(const char* path)
{
    if (path == NULL) {
        path = "";
    }

    const char* extension = taPathFileName(path);
    const char* lastOccurance = NULL;

    // Just find the last '.' and return.
    while (extension[0] != '\0') {
        if (extension[0] == '.') {
            extension += 1;
            lastOccurance = extension;
        }

        extension += 1;
    }

    return (lastOccurance != NULL) ? lastOccurance : extension;
}


taBool32 taPathEqual(const char* path1, const char* path2)
{
    if (path1 == NULL || path2 == NULL) {
        return TA_FALSE;
    }

    if (path1 == path2 || (path1[0] == '\0' && path2[0] == '\0')) {
        return TA_TRUE;    // Two empty paths are treated as the same.
    }

    taPathIterator iPath1;
    taPathIterator iPath2;
    if (taPathFirst(path1, &iPath1) && taPathFirst(path2, &iPath2)) {
        taBool32 isPath1Valid;
        taBool32 isPath2Valid;

        do {
            if (!taPathIteratorsEqual(iPath1, iPath2)) {
                return TA_FALSE;
            }

            isPath1Valid = taPathNext(&iPath1);
            isPath2Valid = taPathNext(&iPath2);

        } while (isPath1Valid && isPath2Valid);

        // At this point either iPath1 and/or iPath2 have finished iterating. If both of them are at the end, the two paths are equal.
        return isPath1Valid == isPath2Valid && iPath1.path[iPath1.segment.offset] == '\0' && iPath2.path[iPath2.segment.offset] == '\0';
    }

    return TA_FALSE;
}

taBool32 taPathExtensionEqual(const char* path, const char* extension)
{
    if (path == NULL || extension == NULL) {
        return TA_FALSE;
    }

    const char* ext1 = extension;
    const char* ext2 = taPathExtension(path);

#ifdef _MSC_VER
    return _stricmp(ext1, ext2) == 0;
#else
    return strcasecmp(ext1, ext2) == 0;
#endif
}


taBool32 taPathIsRelative(const char* path)
{
    if (path == NULL) {
        return TA_FALSE;
    }

    taPathIterator seg;
    if (taPathFirst(path, &seg)) {
        return !taPathIsRootSegment(seg.path, seg.segment);
    }

    // We'll get here if the path is empty. We consider this to be a relative path.
    return TA_TRUE;
}

taBool32 taPathIsAbsolute(const char* path)
{
    return !taPathIsRelative(path);
}


size_t taPathBasePath(char* pathOut, size_t pathOutSize, const char* path)
{
    if (pathOut != NULL && pathOutSize > 0) {
        pathOut[0] = '\0';
    }

    if (path == NULL) {
        return 0;
    }

    const char* pathorig = path;
    const char* baseend = path;

    // We just loop through the path until we find the last slash.
    while (path[0] != '\0') {
        if (path[0] == '/' || path[0] == '\\') {
            baseend = path;
        }

        path += 1;
    }

    // Now we just loop backwards until we hit the first non-slash (this handles situations where there may be multiple path separators such as "C:/MyFolder////MyFile").
    while (baseend > path) {
        if (baseend[0] != '/' && baseend[0] != '\\') {
            break;
        }

        baseend -= 1;
    }

    size_t pathOutLen = (baseend - pathorig);
    if (pathOut != NULL) {
        ta_strncpy_s(pathOut, pathOutSize, pathorig, pathOutLen);
    }

    return (baseend - pathorig) + 1;    // +1 for null terminator
}

size_t taPathFileNameWithoutExtension(char* pathOut, size_t pathOutSize, const char* path)
{
    return taPathRemoveExtension(pathOut, pathOutSize, taPathFileName(path));
}


static size_t taPathAppend_Internal(char* pathOut, size_t pathOutSize, const char* base, const char* other, size_t otherLen)
{
    if (base == NULL) {
        base = "";
    }

    if (other == NULL) {
        other = "";
        otherLen = 0;
    }

    // It only ever makes sense to "append" an absolute path to a blank path. If the other path is absolute, but the base
    // path is not blank we need to return an error because it just doesn't make sense to do this.
    if (taPathIsAbsolute(other) && base[0] != '\0') {
        return 0;
    }

    if (other[0] == '\0') {
        if (pathOut != NULL) {
            if (pathOut != base) {
                ta_strcpy_s(pathOut, pathOutSize, base);
            }
        }
        return strlen(base) + 1;    // +1 for null terminator.
    }


    size_t path1Length = strlen(base);
    size_t path2Length = otherLen;
    size_t slashLength = 0;

    if (path1Length > 0 && base[path1Length-1] != '/' && base[path1Length-1] != '\\') {
        slashLength = 1;
    }

    size_t pathOutLength = path1Length + slashLength + path2Length;

    if (pathOut != NULL) {
        if (pathOutLength+1 <= pathOutSize) {
            if (pathOut != base) {
                ta_strncpy_s(pathOut, pathOutSize, base, path1Length);
            }
            ta_strncpy_s(pathOut + path1Length,               pathOutSize - path1Length,               "/",   slashLength);
            ta_strncpy_s(pathOut + path1Length + slashLength, pathOutSize - path1Length - slashLength, other, path2Length);
        }
    }

    return pathOutLength + 1;   // +1 for null terminator.
}

size_t taPathAppend(char* pathOut, size_t pathOutSize, const char* base, const char* other)
{
    if (other == NULL) {
        other = "";
    }

    return taPathAppend_Internal(pathOut, pathOutSize, base, other, strlen(other));
}

size_t taPathAppendIterator(char* pathOut, size_t pathOutSize, const char* base, taPathIterator i)
{
    return taPathAppend_Internal(pathOut, pathOutSize, base, i.path + i.segment.offset, i.segment.length);
}

size_t taPathAppendExtension(char* pathOut, size_t pathOutSize, const char* base, const char* extension)
{
    if (base == NULL) {
        base = "";
    }

    if (extension == NULL) {
        extension = "";
    }

    if (extension[0] == '\0') {
        if (pathOut != NULL) {
            if (pathOut != base) {
                ta_strcpy_s(pathOut, pathOutSize, base);
            }
        }
        return strlen(base) + 1;    // +1 for null terminator.
    }


    size_t baseLength = strlen(base);
    size_t extLength  = strlen(extension);

    size_t pathOutLength = baseLength + 1 + extLength;

    if (pathOut != NULL) {
        if (pathOutLength+1 <= pathOutSize) {
            if (pathOut != base) {
                ta_strcpy_s(pathOut + 0, pathOutSize - 0, base);
            }
            ta_strcpy_s(pathOut + baseLength,     pathOutSize - baseLength,     ".");
            ta_strcpy_s(pathOut + baseLength + 1, pathOutSize - baseLength - 1, extension);
        }
    }

    return pathOutLength + 1;   // +1 for null terminator.
}


size_t taPathClean_TryWrite(taPathIterator* iterators, unsigned int iteratorCount, char* pathOut, size_t pathOutSize, unsigned int ignoreCounter)
{
    if (iteratorCount == 0) {
        return 0;
    }

    taPathIterator isegment = iterators[iteratorCount - 1];


    // If this segment is a ".", we ignore it. If it is a ".." we ignore it and increment "ignoreCount".
    taBool32 ignoreThisSegment = ignoreCounter > 0 && isegment.segment.length > 0;

    if (isegment.segment.length == 1 && isegment.path[isegment.segment.offset] == '.') {
        // "."
        ignoreThisSegment = TA_TRUE;
    } else {
        if (isegment.segment.length == 2 && isegment.path[isegment.segment.offset] == '.' && isegment.path[isegment.segment.offset + 1] == '.') {
            // ".."
            ignoreThisSegment = TA_TRUE;
            ignoreCounter += 1;
        } else {
            // It's a regular segment, so decrement the ignore counter.
            if (ignoreCounter > 0) {
                ignoreCounter -= 1;
            }
        }
    }


    // The previous segment needs to be written before we can write this one.
    size_t bytesWritten = 0;

    taPathIterator prev = isegment;
    if (!taPathPrev(&prev)) {
        if (iteratorCount > 1) {
            iteratorCount -= 1;
            prev = iterators[iteratorCount - 1];
        } else {
            prev.path           = NULL;
            prev.segment.offset = 0;
            prev.segment.length = 0;
        }
    }

    if (prev.segment.length > 0) {
        iterators[iteratorCount - 1] = prev;
        bytesWritten = taPathClean_TryWrite(iterators, iteratorCount, pathOut, pathOutSize, ignoreCounter);
    }


    if (!ignoreThisSegment) {
        if (pathOut != NULL) {
            pathOut += bytesWritten;
            if (pathOutSize >= bytesWritten) {
                pathOutSize -= bytesWritten;
            } else {
                pathOutSize = 0;
            }
        }

        if (bytesWritten > 0) {
            if (pathOut != NULL) {
                pathOut[0] = '/';
                pathOut += 1;
                if (pathOutSize >= 1) {
                    pathOutSize -= 1;
                } else {
                    pathOutSize = 0;
                }
            }
            bytesWritten += 1;
        }

        if (pathOut != NULL) {
            ta_strncpy_s(pathOut, pathOutSize, isegment.path + isegment.segment.offset, isegment.segment.length);
        }
        bytesWritten += isegment.segment.length;
    }

    return bytesWritten;
}

size_t taPathClean(char* pathOut, size_t pathOutSize, const char* path)
{
    if (path == NULL) {
        return 0;
    }

    taPathIterator last;
    if (taPathLast(path, &last)) {
        size_t bytesWritten = 0;
        if (path[0] == '/') {
            if (pathOut != NULL && pathOutSize > 1) {
                pathOut[0] = '/';
            }
            bytesWritten = 1;
        }

        if (pathOut == NULL || pathOutSize <= bytesWritten) {
            bytesWritten += taPathClean_TryWrite(&last, 1, NULL, 0, 0);
        } else {
            bytesWritten += taPathClean_TryWrite(&last, 1, pathOut + bytesWritten, pathOutSize - bytesWritten - 1, 0);  // -1 to ensure there is enough room for a null terminator later on.
        }
        
        if (pathOut != NULL && pathOutSize > bytesWritten) {
            pathOut[bytesWritten] = '\0';
        }

        return bytesWritten + 1;
    }

    return 0;
}

size_t taPathAppendAndClean(char* pathOut, size_t pathOutSize, const char* base, const char* other)
{
    if (base == NULL || other == NULL) {
        return 0;
    }

    taPathIterator last[2] = {
        {NULL, {0, 0}},
        {NULL, {0, 0}}
    };

    taBool32 isPathEmpty0 = !taPathLast(base,  last + 0);
    taBool32 isPathEmpty1 = !taPathLast(other, last + 1);

    int iteratorCount = !isPathEmpty0 + !isPathEmpty1;
    if (iteratorCount == 0) {
        return 0;   // Both input strings are empty.
    }

    size_t bytesWritten = 0;
    if (base[0] == '/') {
        if (pathOut != NULL && pathOutSize > 1) {
            pathOut[0] = '/';
        }
        bytesWritten = 1;
    }

    if (pathOut == NULL || pathOutSize <= bytesWritten) {
        bytesWritten += taPathClean_TryWrite(last, 2, NULL, 0, 0);
    } else {
        bytesWritten += taPathClean_TryWrite(last, 2, pathOut + bytesWritten, pathOutSize - bytesWritten - 1, 0);  // -1 to ensure there is enough room for a null terminator later on.
    }

    if (pathOut != NULL && pathOutSize > bytesWritten) {
        pathOut[bytesWritten] = '\0';
    }

    return bytesWritten + 1;
}


size_t taPathRemoveExtension(char* pathOut, size_t pathOutSize, const char* path)
{
    if (path == NULL) {
        path = "";
    }

    const char* extension = taPathExtension(path);
    if (extension != NULL && extension[0] != '\0') {
        extension -= 1; // -1 to ensure the dot is removed as well.
    }

    size_t pathOutLength = (size_t)(extension - path);

    if (pathOut != NULL) {
        ta_strncpy_s(pathOut, pathOutSize, path, pathOutLength);
    }

    return pathOutLength;
}

size_t taPathRemoveFileName(char* pathOut, size_t pathOutSize, const char* path)
{
    if (path == NULL) {
        path = "";
    }

    // We just create an iterator that starts at the last segment. We then move back one and place a null terminator at the end of
    // that segment. That will ensure the resulting path is not left with a slash.
    taPathIterator iLast;
    if (!taPathLast(path, &iLast)) {
        return 0;   // The path is empty.
    }

    // Don't remove root segments.
    if (taPathIsRootSegment(iLast.path, iLast.segment)) {
        return 0;
    }

    // If the last segment (the file name portion of the path) is the only segment, just return an empty string. Otherwise we copy
    // up to the end of the second last segment.
    taPathIterator iSecondLast = iLast;
    if (taPathPrev(&iSecondLast)) {
        size_t pathOutLength;
        if (taPathIsUnixStyleRootSegment(iSecondLast.path, iSecondLast.segment)) {
            pathOutLength = iLast.segment.offset;
        } else {
            pathOutLength = iSecondLast.segment.offset + iSecondLast.segment.length;
        }

        if (pathOut != NULL) {
            ta_strncpy_s(pathOut, pathOutSize, path, pathOutLength);
        }

        return pathOutLength + 1;
    } else {
        if (pathOut != NULL && pathOutSize > 0) {
            pathOut[0] = '\0';
        }

        return 1;   // Return 1 because we need to include the null terminator.
    }
}

size_t taPathRemoveFileNameInPlace(char* path)
{
    if (path == NULL) {
        return 0;
    }

    // We just create an iterator that starts at the last segment. We then move back one and place a null terminator at the end of
    // that segment. That will ensure the resulting path is not left with a slash.
    taPathIterator iLast;
    if (!taPathLast(path, &iLast)) {
        return 0;   // The path is empty.
    }

    // Don't remove root segments.
    if (taPathIsRootSegment(iLast.path, iLast.segment)) {
        return 0;
    }

    // If the last segment (the file name portion of the path) is the only segment, just return an empty string. Otherwise we copy
    // up to the end of the second last segment.
    taPathIterator iSecondLast = iLast;
    if (taPathPrev(&iSecondLast)) {
        size_t pathOutLength;
        if (taPathIsUnixStyleRootSegment(iSecondLast.path, iSecondLast.segment)) {
            pathOutLength = iLast.segment.offset;
        } else {
            pathOutLength = iSecondLast.segment.offset + iSecondLast.segment.length;
        }

        path[pathOutLength] = '\0';
        return pathOutLength + 1;
    } else {
        path[0] = 0;
        return 1;   // Return 1 because we need to include the null terminator.
    }
}


size_t taPathToRelative(char* pathOut, size_t pathOutSize, const char* absolutePathToMakeRelative, const char* absolutePathToMakeRelativeTo)
{
    // We do this in two phases. The first phase just iterates past each segment of both the path to convert and the
    // base path until we find two that are not equal. The second phase just adds the appropriate ".." segments.

    if (pathOut != NULL && pathOutSize > 0) {
        pathOut[0] = '\0';
    }

    if (!taPathIsAbsolute(absolutePathToMakeRelative) || !taPathIsAbsolute(absolutePathToMakeRelativeTo)) {
        return 0;
    }

    taPathIterator iPath;
    taPathIterator iBase;
    taBool32 isPathEmpty = !taPathFirst(absolutePathToMakeRelative, &iPath);
    taBool32 isBaseEmpty = !taPathFirst(absolutePathToMakeRelativeTo, &iBase);

    if (isPathEmpty && isBaseEmpty) {
        return 0;   // Looks like both paths are empty.
    }


    // Phase 1: Get past the common section.
    int isPathAtEnd = 0;
    int isBaseAtEnd = 0;
    while (!isPathAtEnd && !isBaseAtEnd && taPathIteratorsEqual(iPath, iBase)) {
        isPathAtEnd = !taPathNext(&iPath);
        isBaseAtEnd = !taPathNext(&iBase);
    }

    if (iPath.segment.offset == 0) {
        return 0;   // The path is not relative to the base path.
    }


    // Phase 2: Append ".." segments - one for each remaining segment in the base path.
    size_t pathOutLength = 0;

    if (!taPathAtEnd(iBase)) {
        do {
            if (pathOutLength == 0) {
                // It's the first segment, so we need to ensure we don't lead with a slash.
                if (pathOut != NULL && pathOutLength+2 < pathOutSize) {
                    pathOut[pathOutLength + 0] = '.';
                    pathOut[pathOutLength + 1] = '.';
                }
                pathOutLength += 2;
            } else {
                // It's not the first segment. Make sure we lead with a slash.
                if (pathOut != NULL && pathOutLength+3 < pathOutSize) {
                    pathOut[pathOutLength + 0] = '/';
                    pathOut[pathOutLength + 1] = '.';
                    pathOut[pathOutLength + 2] = '.';
                }
                pathOutLength += 3;
            }
        } while (taPathNext(&iBase));
    }


    // Now we just append whatever is left of the main path. We want the path to be clean, so we append segment-by-segment.
    if (!taPathAtEnd(iPath)) {
        do {
            // Leading slash, if required.
            if (pathOutLength != 0) {
                if (pathOut != NULL && pathOutLength+1 < pathOutSize) {
                    pathOut[pathOutLength] = '/';
                }
                pathOutLength += 1;
            }

            if (pathOut != NULL) {
                ta_strncpy_s(pathOut + pathOutLength, pathOutSize - pathOutLength, iPath.path + iPath.segment.offset, iPath.segment.length);
            }
            pathOutLength += iPath.segment.length;
        } while (taPathNext(&iPath));
    }


    // Always null terminate.
    if (pathOut != NULL && pathOutLength+1 <= pathOutSize) {
        pathOut[pathOutLength] = '\0';
    }

    return pathOutLength + 1;   // +1 for null terminator.
}

size_t taPathToAbsolute(char* pathOut, size_t pathOutSize, const char* relativePathToMakeAbsolute, const char* basePath)
{
    return taPathAppendAndClean(pathOut, pathOutSize, basePath, relativePathToMakeAbsolute);
}


///////////////////////////////////////////////////////////////////////////////
//
// High Level APIs
//
///////////////////////////////////////////////////////////////////////////////

taString taPathFileNameWithoutExtensionStr(const char* path)
{
    size_t len = taPathFileNameWithoutExtension(NULL, 0, path);
    if (len == 0) {
        return NULL;
    }

    taString str = taMallocString(len+1);
    if (str == NULL) {
        return NULL;
    }

    taPathFileNameWithoutExtension(str, len+1, path);
    return str;
}
