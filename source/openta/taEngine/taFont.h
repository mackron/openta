// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct
{
    float u;
    float v;
    float originX;
    float originY;
    float sizeX;
    float sizeY;
} taFontGlyph;

struct taFont
{
    taEngineContext* pEngine;
    float height;
    taFontGlyph glyphs[256];
    taBool32 canBeColored; // Set to true for FNT fonts, false for GAF fonts.
    taTexture* pTexture;
};

taResult taFontLoad(taEngineContext* pEngine, const char* filePath, taFont* pFont);
taResult taFontUnload(taFont* pFont);
taResult taFontMeasureText(taFont* pFont, float scale, const char* text, float* pSizeX, float* pSizeY);
taResult taFontFindCharacterMetrics(taFont* pFont, float scale, const char* text, char c, float* pPosX, float* pPosY, float* pSizeX, float* pSizeY);
