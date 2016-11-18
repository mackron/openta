// Public domain. See "unlicense" statement at the end of this file.

// This file just contains miscellaneous stuff that doesn't really fit anywhere.

//// Memory Stream ////

ta_memory_stream ta_create_memory_stream(void* pData, size_t dataSize)
{
    ta_memory_stream stream;
    stream.pData = pData;
    stream.dataSize = dataSize;
    stream.currentReadPos = 0;

    return stream;
}

size_t ta_memory_stream_read(ta_memory_stream* pStream, void* pDataOut, size_t bytesToRead)
{
    size_t bytesRead = ta_memory_stream_peek(pStream, pDataOut, bytesToRead);
    if (bytesRead == 0) {
        return 0;
    }

    pStream->currentReadPos += bytesRead;
    
    return bytesRead;
}

size_t ta_memory_stream_peek(ta_memory_stream* pStream, void* pDataOut, size_t bytesToRead)
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

ta_bool32 ta_memory_stream_seek(ta_memory_stream* pStream, int64_t bytesToSeek, ta_seek_origin origin)
{
    uint64_t newPos = pStream->currentReadPos;
    if (origin == ta_seek_origin_current)
    {
        if ((int64_t)newPos + bytesToSeek >= 0) {
            newPos = (uint64_t)((int64_t)newPos + bytesToSeek);
        } else {
            // Trying to seek to before the beginning of the file.
            return TA_FALSE;
        }
    }
    else if (origin == ta_seek_origin_start)
    {
        assert(bytesToSeek >= 0);
        newPos = (uint64_t)bytesToSeek;
    }
    else if (origin == ta_seek_origin_end)
    {
        assert(bytesToSeek >= 0);
        if ((uint64_t)bytesToSeek <= pStream->dataSize) {
            newPos = pStream->dataSize - (uint64_t)bytesToSeek;
        } else {
            // Trying to seek to before the beginning of the file.
            return TA_FALSE;
        }
    }
    else
    {
        // Should never get here.
        return TA_FALSE;
    }


    if (newPos > pStream->dataSize) {
        return TA_FALSE;
    }

    pStream->currentReadPos = (size_t)newPos;
    return TA_TRUE;
}

size_t ta_memory_stream_tell(ta_memory_stream* pStream)
{
    if (pStream == NULL) {
        return 0;
    }

    return pStream->currentReadPos;
}

ta_bool32 ta_memory_stream_write_uint32(ta_memory_stream* pStream, uint32_t value)
{
    if (pStream == NULL) {
        return TA_FALSE;
    }

    if ((pStream->dataSize - pStream->currentReadPos) < 4) {
        return TA_FALSE;
    }

    *((uint32_t*)(pStream->pData + pStream->currentReadPos)) = value;
    pStream->currentReadPos += sizeof(uint32_t);

    return TA_TRUE;
}