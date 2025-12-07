#ifndef TEX_TEX_FONTS_H
#define TEX_TEX_FONTS_H

#include <stdint.h>

#ifdef TEX_USE_FONTLIB
#include <fontlibc.h>
typedef fontlib_font_t* TexFontPtr;
#else
typedef void* TexFontPtr;
#endif

typedef struct
{
	TexFontPtr main_font;
	TexFontPtr script_font;
	int main_height;
	int main_baseline;
	int script_height;
	int script_baseline;
} TexFontHandles;

// load font handles from two packs (main, script)
// NULL or empty names default to "TeXFonts" and "TeXScrpt"
// returns 1 on success, 0 on failure
int tex_fonts_load(const char* pack_main, const char* pack_script, TexFontHandles* out);

#endif // TEX_TEX_FONTS_H
