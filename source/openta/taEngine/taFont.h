// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct
{
    float u;
    float v;
    float originX;
    float originY;
    float sizeX;
    float sizeY;
} ta_font_glyph;

struct ta_font
{
    taEngineContext* pEngine;
    float height;
    ta_font_glyph glyphs[256];
    ta_bool32 canBeColored; // Set to true for FNT fonts, false for GAF fonts.
    ta_texture* pTexture;
};

ta_result ta_font_load(taEngineContext* pEngine, const char* filePath, ta_font* pFont);
ta_result ta_font_unload(ta_font* pFont);
ta_result ta_font_measure_text(ta_font* pFont, float scale, const char* text, float* pSizeX, float* pSizeY);
ta_result ta_font_find_character_metrics(ta_font* pFont, float scale, const char* text, char c, float* pPosX, float* pPosY, float* pSizeX, float* pSizeY);