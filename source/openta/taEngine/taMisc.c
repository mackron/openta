// Copyright (C) 2018 David Reid. See included LICENSE file.

// This file just contains miscellaneous stuff that doesn't really fit anywhere.

//// Memory Stream ////

taMemoryStream taCreateMemoryStream(void* pData, size_t dataSize)
{
    taMemoryStream stream;
    stream.pData = pData;
    stream.dataSize = dataSize;
    stream.currentReadPos = 0;

    return stream;
}

size_t taMemoryStreamRead(taMemoryStream* pStream, void* pDataOut, size_t bytesToRead)
{
    size_t bytesRead = taMemoryStreamPeek(pStream, pDataOut, bytesToRead);
    if (bytesRead == 0) {
        return 0;
    }

    pStream->currentReadPos += bytesRead;
    
    return bytesRead;
}

size_t taMemoryStreamPeek(taMemoryStream* pStream, void* pDataOut, size_t bytesToRead)
{
    if (pStream == NULL) {
        return 0;
    }

    size_t bytesRemaining = pStream->dataSize - pStream->currentReadPos;
    if (bytesToRead > bytesRemaining) {
        bytesToRead = bytesRemaining;
    }

    if (bytesToRead > 0) {
        memcpy(pDataOut, pStream->pData + pStream->currentReadPos, bytesToRead);
    }

    return bytesToRead;
}

taBool32 taMemoryStreamSeek(taMemoryStream* pStream, taInt64 bytesToSeek, taSeekOrigin origin)
{
    taUInt64 newPos = pStream->currentReadPos;
    if (origin == taSeekOriginCurrent) {
        if ((taInt64)newPos + bytesToSeek >= 0) {
            newPos = (taUInt64)((taInt64)newPos + bytesToSeek);
        } else {
            // Trying to seek to before the beginning of the file.
            return TA_FALSE;
        }
    } else if (origin == taSeekOriginStart) {
        assert(bytesToSeek >= 0);
        newPos = (taUInt64)bytesToSeek;
    } else if (origin == taSeekOriginEnd) {
        assert(bytesToSeek >= 0);
        if ((taUInt64)bytesToSeek <= pStream->dataSize) {
            newPos = pStream->dataSize - (taUInt64)bytesToSeek;
        } else {
            // Trying to seek to before the beginning of the file.
            return TA_FALSE;
        }
    } else {
        // Should never get here.
        return TA_FALSE;
    }

    if (newPos > pStream->dataSize) {
        return TA_FALSE;
    }

    pStream->currentReadPos = (size_t)newPos;
    return TA_TRUE;
}

size_t taMemoryStreamTell(taMemoryStream* pStream)
{
    if (pStream == NULL) {
        return 0;
    }

    return pStream->currentReadPos;
}

taBool32 taMemoryStreamWriteUInt32(taMemoryStream* pStream, taUInt32 value)
{
    if (pStream == NULL) {
        return TA_FALSE;
    }

    if ((pStream->dataSize - pStream->currentReadPos) < 4) {
        return TA_FALSE;
    }

    *((taUInt32*)(pStream->pData + pStream->currentReadPos)) = value;
    pStream->currentReadPos += sizeof(taUInt32);

    return TA_TRUE;
}


//// Timing ////
#ifdef _WIN32
static LARGE_INTEGER g_taTimerFrequency = {{0}};
void taTimerInit(taTimer* pTimer)
{
    if (g_taTimerFrequency.QuadPart == 0) {
        QueryPerformanceFrequency(&g_taTimerFrequency);
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    pTimer->counter = (taUInt64)counter.QuadPart;
}

double taTimerTick(taTimer* pTimer)
{
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    taUInt64 newTimeCounter = counter.QuadPart;
    taUInt64 oldTimeCounter = pTimer->counter;

    pTimer->counter = newTimeCounter;

    return (newTimeCounter - oldTimeCounter) / (double)g_taTimerFrequency.QuadPart;
}
#else
void taTimerInit(taTimer* pTimer)
{
    struct timespec newTime;
    clock_gettime(CLOCK_MONOTONIC, &newTime);

    pTimer->counter = (newTime.tv_sec * 1000000000LL) + newTime.tv_nsec;
}

double taTimerTick(taTimer* pTimer)
{
    struct timespec newTime;
    clock_gettime(CLOCK_MONOTONIC, &newTime);

    taUInt64 newTimeCounter = (newTime.tv_sec * 1000000000LL) + newTime.tv_nsec;
    taUInt64 oldTimeCounter = pTimer->counter;

    pTimer->counter = newTimeCounter;

    return (newTimeCounter - oldTimeCounter) / 1000000000.0;
}
#endif