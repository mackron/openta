// Public domain. See "unlicense" statement at the end of this file.

bool ta_texture_packer__find_slot(ta_texture_packer* pPacker, uint32_t width, uint32_t height, uint32_t* pPosXOut, uint32_t* pPosYOut)
{
    assert(pPacker != NULL);
    assert(pPosXOut != NULL);
    assert(pPosYOut != NULL);

    if (width > pPacker->width || height > pPacker->height) {
        return false;
    }


    // Finding a slot is fairly straight forward. There's just a cursor that runs along the x and y axis and we just try to place
    // the image such that the top left of it is sitting on that point.

    // Check if there is room on the x axis. If there isn't we'll need to move the cursor down.
    if (pPacker->cursorPosX + width <= pPacker->width)
    {
        // There is enough room on the x axis, so now check the y axis.
        if (pPacker->cursorPosY + height <= pPacker->height)
        {
            // There is enough room on the y axis as well. Place the sub-texture here.
            *pPosXOut = pPacker->cursorPosX;
            *pPosYOut = pPacker->cursorPosY;

            pPacker->cursorPosX += width;
            if (pPacker->currentRowHeight < height) {
                pPacker->currentRowHeight = height;
            }

            return true;
        }
        else
        {
            // There is not enough room on the y axis. If it can't fit at this point it will never fit.
            return false;
        }
    }
    else
    {
        // There is not enough room on the x axis, but there may be room on the next row.
        if (pPacker->cursorPosY + pPacker->currentRowHeight + height <= pPacker->height)
        {
            // There is enough room on the x axis if we just move to the next row.
            pPacker->cursorPosX = 0;
            pPacker->cursorPosY = pPacker->cursorPosY + pPacker->currentRowHeight;

            *pPosXOut = pPacker->cursorPosX;
            *pPosYOut = pPacker->cursorPosY;

            pPacker->cursorPosX += width;
            pPacker->currentRowHeight = height;

            return true;
        }
        else
        {
            // Going to the next row will not leave enough room. It will never fit.
            return false;
        }
    }
}

bool ta_texture_packer__copy_image_data(ta_texture_packer* pPacker, ta_texture_packer_slot* pSlot, const uint8_t* pSubTextureData)
{
    assert(pPacker != NULL);
    assert(pSlot != NULL);
    assert(pSubTextureData != NULL);

    uint32_t srcStride = pPacker->bpp * pSlot->width;
    uint32_t dstStride = pPacker->bpp * pPacker->width;

    for (uint32_t y = 0; y < pSlot->height; ++y)
    {
        const uint8_t* pSrcRow = pSubTextureData + (y * srcStride);
        uint8_t* pDstRow = pPacker->pImageData + ((pSlot->posY + y) * dstStride);
        memcpy(pDstRow + pSlot->posX, pSrcRow, srcStride);
    }

    return true;
}


bool ta_texture_packer_init(ta_texture_packer* pPacker, uint32_t width, uint32_t height, uint32_t bytesPerPixel)
{
    if (pPacker == NULL || width == 0 || height == 0) {
        return false;
    }

    memset(pPacker, 0, sizeof(*pPacker));
    pPacker->width = width;
    pPacker->height = height;
    pPacker->bpp = bytesPerPixel;
    pPacker->pImageData = calloc(1, width * height * bytesPerPixel);   // <-- calloc this so the background is black. Important for debugging.
    if (pPacker->pImageData == NULL) {
        return false;
    }

    return true;
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

    // Clear the image data to black to make the background easy to see during debugging.
    memset(pPacker->pImageData, 0, pPacker->width * pPacker->height * pPacker->bpp);
}

bool ta_texture_packer_pack_subtexture(ta_texture_packer* pPacker, uint32_t width, uint32_t height, const void* pSubTextureData, ta_texture_packer_slot* pSlotOut)
{
    if (pPacker == NULL || pSubTextureData == NULL) {
        return false;
    }

    ta_texture_packer_slot slot;
    if (!ta_texture_packer__find_slot(pPacker, width, height, &slot.posX, &slot.posY)) {
        return false;
    }

    slot.width = width;
    slot.height = height;
    if (!ta_texture_packer__copy_image_data(pPacker, &slot, pSubTextureData)) {
        return false;
    }


    if (pSlotOut) {
        *pSlotOut = slot;
    }

    return true;
}