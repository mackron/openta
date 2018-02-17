// Copyright (C) 2018 David Reid. See included LICENSE file.

ta_bool32 ta_texture_packer__find_slot(ta_texture_packer* pPacker, uint32_t width, uint32_t height, uint32_t* pPosXOut, uint32_t* pPosYOut)
{
    assert(pPacker != NULL);
    assert(pPosXOut != NULL);
    assert(pPosYOut != NULL);

    ta_uint32 edge = (pPacker->flags & (TA_TEXTURE_PACKER_FLAG_HARD_EDGE | TA_TEXTURE_PACKER_FLAG_TRANSPARENT_EDGE)) ? 1 : 0;
    ta_uint32 outerWidth  = width  + (edge*2);
    ta_uint32 outerHeight = height + (edge*2);

    if (outerWidth > pPacker->width || outerHeight > pPacker->height) {
        return TA_FALSE;
    }


    // Finding a slot is fairly straight forward. There's just a cursor that runs along the x and y axis and we just try to place
    // the image such that the top left of it is sitting on that point.

    // Check if there is room on the x axis. If there isn't we'll need to move the cursor down.
    if (pPacker->cursorPosX + outerWidth <= pPacker->width)
    {
        // There is enough room on the x axis, so now check the y axis.
        if (pPacker->cursorPosY + outerHeight <= pPacker->height)
        {
            // There is enough room on the y axis as well. Place the sub-texture here.
            *pPosXOut = pPacker->cursorPosX + edge;
            *pPosYOut = pPacker->cursorPosY + edge;

            pPacker->cursorPosX += outerWidth;
            if (pPacker->currentRowHeight < outerHeight) {
                pPacker->currentRowHeight = outerHeight;
            }

            return TA_TRUE;
        }
        else
        {
            // There is not enough room on the y axis. If it can't fit at this point it will never fit.
            return TA_FALSE;
        }
    }
    else
    {
        // There is not enough room on the x axis, but there may be room on the next row.
        if (pPacker->cursorPosY + pPacker->currentRowHeight + outerHeight <= pPacker->height)
        {
            // There is enough room on the x axis if we just move to the next row.
            pPacker->cursorPosX = 0;
            pPacker->cursorPosY = pPacker->cursorPosY + pPacker->currentRowHeight;

            *pPosXOut = pPacker->cursorPosX + edge;
            *pPosYOut = pPacker->cursorPosY + edge;

            pPacker->cursorPosX += outerWidth;
            pPacker->currentRowHeight = outerHeight;

            return TA_TRUE;
        }
        else
        {
            // Going to the next row will not leave enough room. It will never fit.
            return TA_FALSE;
        }
    }
}

ta_bool32 ta_texture_packer__copy_image_data(ta_texture_packer* pPacker, const ta_texture_packer_slot* pSlot, const uint8_t* pSubTextureData)
{
    assert(pPacker != NULL);
    assert(pSlot != NULL);
    assert(pSubTextureData != NULL);

    if (pSlot->width == 0 || pSlot->height == 0) {
        return TA_TRUE;
    }

    uint32_t srcStride = pPacker->bpp * pSlot->width;
    uint32_t dstStride = pPacker->bpp * pPacker->width;

    for (uint32_t y = 0; y < pSlot->height; ++y)
    {
        const uint8_t* pSrcRow = pSubTextureData + (y * srcStride);
        uint8_t* pDstRow = pPacker->pImageData + ((pSlot->posY + y) * dstStride);
        memcpy(pDstRow + pSlot->posX*pPacker->bpp, pSrcRow, srcStride);
    }

    // Do the edge if applicable. The edge is equal to the outside edge of the main part of the image. Note that we're
    // deliberately not checking the TRANSPARENT_EDGE flag because the texture packer is always initialized to transparency
    // by default which therefore means we don't need to do anything special.
    ta_uint32 edge = (pPacker->flags & TA_TEXTURE_PACKER_FLAG_HARD_EDGE) ? 1 : 0;
    if (edge > 0) {
        // Top and bottom edges.
        const uint8_t* pSrcRow0 = pSubTextureData     + (0                               * srcStride);
        const uint8_t* pSrcRow1 = pSubTextureData     + ((pSlot->height-1)               * srcStride);
              uint8_t* pDstRow0 = pPacker->pImageData + ((pSlot->posY - 1)               * dstStride);
              uint8_t* pDstRow1 = pPacker->pImageData + ((pSlot->posY + (pSlot->height)) * dstStride);

        memcpy(pDstRow0 + pSlot->posX*pPacker->bpp, pSrcRow0, srcStride);
        memcpy(pDstRow1 + pSlot->posX*pPacker->bpp, pSrcRow1, srcStride);

        // Side edges.
        for (ta_uint32 y = pSlot->posY-1; y < pSlot->posY+pSlot->height+1; ++y) {
            uint8_t* pDstRow = pPacker->pImageData + (y * dstStride) + ((pSlot->posX-1) * pPacker->bpp);
            uint8_t* pDstOuter0 = pDstRow;
            uint8_t* pDstInner0 = pDstRow + pPacker->bpp;
            uint8_t* pDstOuter1 = pDstRow + ((pSlot->width+1) * pPacker->bpp);
            uint8_t* pDstInner1 = pDstRow + ((pSlot->width+0) * pPacker->bpp);

            memcpy(pDstOuter0, pDstInner0, pPacker->bpp);
            memcpy(pDstOuter1, pDstInner1, pPacker->bpp);
        }
    }

    return TA_TRUE;
}


ta_bool32 ta_texture_packer_init(ta_texture_packer* pPacker, uint32_t width, uint32_t height, uint32_t bytesPerPixel, ta_uint32 flags)
{
    if (pPacker == NULL || width == 0 || height == 0) {
        return TA_FALSE;
    }

    memset(pPacker, 0, sizeof(*pPacker));
    pPacker->width = width;
    pPacker->height = height;
    pPacker->bpp = bytesPerPixel;
    pPacker->flags = flags;
    pPacker->pImageData = malloc(width * height * bytesPerPixel);
    if (pPacker->pImageData == NULL) {
        return TA_FALSE;
    }

    if (pPacker->bpp == 1) {
        memset(pPacker->pImageData, TA_TRANSPARENT_COLOR, pPacker->width * pPacker->height * pPacker->bpp);
    } else {
        memset(pPacker->pImageData, 0, pPacker->width * pPacker->height * pPacker->bpp);
    }

    return TA_TRUE;
}

void ta_texture_packer_uninit(ta_texture_packer* pPacker)
{
    if (pPacker == NULL) {
        return;
    }

    free(pPacker->pImageData);
}

void ta_texture_packer_reset(ta_texture_packer* pPacker)
{
    if (pPacker == NULL) {
        return;
    }

    pPacker->cursorPosX = 0;
    pPacker->cursorPosY = 0;
    pPacker->currentRowHeight = 0;

    // Clear the image data to transparency.
    if (pPacker->bpp == 1) {
        memset(pPacker->pImageData, TA_TRANSPARENT_COLOR, pPacker->width * pPacker->height * pPacker->bpp);
    } else {
        memset(pPacker->pImageData, 0, pPacker->width * pPacker->height * pPacker->bpp);
    }
}

ta_bool32 ta_texture_packer_pack_subtexture(ta_texture_packer* pPacker, uint32_t width, uint32_t height, const void* pSubTextureData, ta_texture_packer_slot* pSlotOut)
{
    if (pSlotOut) ta_zero_object(pSlotOut);
    if (pPacker == NULL) {
        return TA_FALSE;
    }

    ta_texture_packer_slot slot;
    if (!ta_texture_packer__find_slot(pPacker, width, height, &slot.posX, &slot.posY)) {
        return TA_FALSE;
    }

    slot.width = width;
    slot.height = height;

    if (pSubTextureData != NULL) {
        if (!ta_texture_packer__copy_image_data(pPacker, &slot, pSubTextureData)) {
            return TA_FALSE;
        }
    }

    if (pSlotOut) *pSlotOut = slot;
    return TA_TRUE;
}

ta_bool32 ta_texture_packer_is_empty(const ta_texture_packer* pPacker)
{
    if (pPacker == NULL) return TA_FALSE;
    return pPacker->cursorPosX == 0 && pPacker->cursorPosY;
}
