// Copyright (C) 2018 David Reid. See included LICENSE file.

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

taBool32 ta_memory_stream_seek(ta_memory_stream* pStream, taInt64 bytesToSeek, taSeekOrigin origin)
{
    taUInt64 newPos = pStream->currentReadPos;
    if (origin == taSeekOriginCurrent)
    {
        if ((taInt64)newPos + bytesToSeek >= 0) {
            newPos = (taUInt64)((taInt64)newPos + bytesToSeek);
        } else {
            // Trying to seek to before the beginning of the file.
            return TA_FALSE;
        }
    }
    else if (origin == taSeekOriginStart)
    {
        assert(bytesToSeek >= 0);
        newPos = (taUInt64)bytesToSeek;
    }
    else if (origin == taSeekOriginEnd)
    {
        assert(bytesToSeek >= 0);
        if ((taUInt64)bytesToSeek <= pStream->dataSize) {
            newPos = pStream->dataSize - (taUInt64)bytesToSeek;
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

taBool32 ta_memory_stream_write_uint32(ta_memory_stream* pStream, taUInt32 value)
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