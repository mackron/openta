// Copyright (C) 2016 David Reid. See included LICENSE file.

typedef struct
{
    float u;
    float v;
    float width;
} ta_font_glyph;

struct ta_font
{
    ta_game* pGame;
    float height;
    ta_font_glyph glyphs[256];
    ta_texture* pTexture;
};

ta_result ta_font_load(ta_game* pGame, const char* filePath, ta_font* pFont);
ta_result ta_font_unload(ta_font* pFont);
ta_result ta_font_measure_text(ta_font* pFont, float scale, const char* text, float* pSizeX, float* pSizeY);