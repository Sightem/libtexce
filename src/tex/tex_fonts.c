#include "tex_fonts.h"

#ifdef TEX_USE_FONTLIB
#include <fontlibc.h>
#endif


#ifndef TEX_USE_FONTLIB
static const int HOST_MAIN_ASC = 8;
static const int HOST_MAIN_DESC = 4;
static const int HOST_SCRIPT_ASC = 6;
static const int HOST_SCRIPT_DESC = 3;
static char g_sentinel_main = 0;
static char g_sentinel_script = 0;
#endif

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int tex_fonts_load(const char* pack_main, const char* pack_script, TexFontHandles* out)
{
	if (!out)
		return 0;

#ifdef TEX_USE_FONTLIB
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
#else
	(void)pack_main;
	(void)pack_script;
	out->main_font = (TexFontPtr)&g_sentinel_main;
	out->script_font = (TexFontPtr)&g_sentinel_script;
	out->main_baseline = HOST_MAIN_ASC;
	out->main_height = HOST_MAIN_ASC + HOST_MAIN_DESC;
	out->script_baseline = HOST_SCRIPT_ASC;
	out->script_height = HOST_SCRIPT_ASC + HOST_SCRIPT_DESC;
	return 1;
#endif
}
