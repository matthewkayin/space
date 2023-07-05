#include "text.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

int next_powerof2(int value) {
    int rval = 1;
    while (rval < value) {
        rval <<= 1;
    }

    return rval;
}

void make_display_list(FT_Face face, char c, unsigned int list_base, unsigned int* texture_base) {
    if (FT_Load_Glyph(face, FT_Get_Char_Index(face, c), FT_LOAD_DEFAULT)) {
        printf("FT_Load_Glyph failed\n");
        return;
    }

    FT_Glyph glyph;
    if (FT_Get_Glyph(face->glyph, &glyph)) {
        printf("FT_Get_Glyph failed\n");
        return;
    }
}
