#include "tex_fonts.h"
#include <fontlibc.h>

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int tex_fonts_load(const char* pack_main, const char* pack_script, TexFontHandles* out)
{
	if (!out)
		return 0;

	const char* pmain = (pack_main && pack_main[0]) ? pack_main : "TeXFonts";
	const char* pscr = (pack_script && pack_script[0]) ? pack_script : "TeXScrpt";
	fontlib_font_t* mf = fontlib_GetFontByIndex(pmain, 0);
	fontlib_font_t* sf = fontlib_GetFontByIndex(pscr, 0);
	if (!mf || !sf)
	{
		return 0;
	}
	out->main_font = mf;
	out->script_font = sf;
	out->main_baseline = mf->baseline_height;
	out->main_height = mf->height;
	out->script_baseline = sf->baseline_height;
	out->script_height = sf->height;
	return 1;
}

