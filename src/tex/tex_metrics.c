#include "tex_metrics.h"
#include <limits.h>
#include <string.h>
#include "tex_fonts.h"

#ifdef TEX_USE_FONTLIB
#include <fontlibc.h>
#endif


typedef struct
{
	int16_t main_asc, main_desc;
	int16_t script_asc, script_desc;
#ifdef TEX_USE_FONTLIB
	fontlib_font_t* mf;
	fontlib_font_t* sf;
#endif
	int use_fontlib;
} TexMetricsState;

static TexMetricsState g_state;

#ifdef TEX_USE_FONTLIB
FontRole g_tex_metrics_current_role = (FontRole)-1;
#endif

void tex_metrics_reset(void)
{
	memset(&g_state, 0, sizeof(g_state));
	g_state.use_fontlib = 0;
#ifdef TEX_USE_FONTLIB
	g_tex_metrics_current_role = (FontRole)-1; // force first SetFont
#endif
}

int16_t tex_metrics_math_axis(void)
{
#ifdef TEX_USE_FONTLIB
	return g_state.mf ? (int16_t)g_state.mf->x_height : 0;
#else
	return 0;
#endif
}

#include "tex_internal.h"
void tex_metrics_init(struct TeX_Layout* layout)
{
	tex_metrics_reset();
	TexFontHandles fh;

	const char* pack_main = layout ? layout->cfg.pack : NULL;

	int result = tex_fonts_load(pack_main, NULL, &fh);

	if (result)
	{
		g_state.main_asc = (int16_t)fh.main_baseline;
		g_state.main_desc = (int16_t)(fh.main_height - fh.main_baseline);
		g_state.script_asc = (int16_t)fh.script_baseline;
		g_state.script_desc = (int16_t)(fh.script_height - fh.script_baseline);
#ifdef TEX_USE_FONTLIB
		g_state.mf = (fontlib_font_t*)fh.main_font;
		g_state.sf = (fontlib_font_t*)fh.script_font;
		g_state.use_fontlib = 1;
		g_tex_metrics_current_role = (FontRole)-1; // reset cache after (re)load
#else
		g_state.use_fontlib = 0;
#endif
		tex_reserved_init();
	}
	else
	{
		if (layout)
		{
			TEX_SET_ERROR(layout, TEX_ERR_FONT, "Failed to load fonts", 0);
		}
	}
}

int16_t tex_metrics_asc(FontRole role)
{
	return (int16_t)((role == FONTROLE_SCRIPT) ? g_state.script_asc : g_state.main_asc);
}

int16_t tex_metrics_desc(FontRole role)
{
	return (int16_t)((role == FONTROLE_SCRIPT) ? g_state.script_desc : g_state.main_desc);
}


int16_t tex_metrics_text_width(const char* s, FontRole role)
{
#ifdef TEX_USE_FONTLIB
	if (g_state.use_fontlib && g_state.mf && g_state.sf)
	{
		// ensure font is active (cached)
		if (((role == FONTROLE_SCRIPT) ? g_state.sf : g_state.mf) == NULL)
		{
			return 0;
		}
		if (g_tex_metrics_current_role != role)
		{
			fontlib_font_t* font = (role == FONTROLE_SCRIPT) ? g_state.sf : g_state.mf;
#if defined(__TICE__)
			if (!fontlib_SetFont(font, (fontlib_load_options_t)0))
			{
				return 0;
			}
#else
			fontlib_SetFont(font, (fontlib_load_options_t)0);
#endif
			g_tex_metrics_current_role = role;
		}
		return (int16_t)fontlib_GetStringWidth(s ? s : "");
	}
#endif
	(void)s;
	(void)role;
	return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int16_t tex_metrics_text_width_n(const char* s, int len, FontRole role)
{
#ifdef TEX_USE_FONTLIB
	if (g_state.use_fontlib && g_state.mf && g_state.sf)
	{
		if (((role == FONTROLE_SCRIPT) ? g_state.sf : g_state.mf) == NULL)
			return 0;
		if (g_tex_metrics_current_role != role)
		{
			fontlib_font_t* font = (role == FONTROLE_SCRIPT) ? g_state.sf : g_state.mf;
			if (!fontlib_SetFont(font, (fontlib_load_options_t)0))
				return 0;
			fontlib_SetFont(font, (fontlib_load_options_t)0);
			g_tex_metrics_current_role = role;
		}
		if (!s || len <= 0)
			return 0;


		return (int16_t)fontlib_GetStringWidthL(s, (size_t)len);
	}
#endif
	(void)role;
	(void)s;
	(void)len;
	return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int16_t tex_metrics_glyph_width(unsigned int glyph, FontRole role)
{
#ifdef TEX_USE_FONTLIB
	if (g_state.use_fontlib && g_state.mf && g_state.sf)
	{
		char ch[2];
		ch[0] = (char)(glyph & 0xFF);
		ch[1] = '\0';

		if (((role == FONTROLE_SCRIPT) ? g_state.sf : g_state.mf) == NULL)
			return 0;

		if (g_tex_metrics_current_role != role)
		{
			fontlib_font_t* font = (role == FONTROLE_SCRIPT) ? g_state.sf : g_state.mf;
			fontlib_SetFont(font, (fontlib_load_options_t)0);

			g_tex_metrics_current_role = role;
		}

		// HACK: set threshold to 1
		char old_first = fontlib_GetFirstPrintableCodePoint();
		fontlib_SetFirstPrintableCodePoint((char)0x01);

		int16_t result = (int16_t)fontlib_GetStringWidth(ch);
		fontlib_SetFirstPrintableCodePoint(old_first);
		return result;
	}
#endif
	(void)glyph;
	(void)role;
	return 0;
}

Node g_reserved_nodes[TEX_RESERVED_COUNT];

void tex_reserved_init(void)
{
	// main role glyphs (0-127)
	for (int i = 0; i < 128; i++)
	{
		Node* n = &g_reserved_nodes[i];
		n->type = N_GLYPH;
		n->flags = 0;
		n->data.glyph = (uint16_t)i;

		n->w = tex_metrics_glyph_width((unsigned int)i, FONTROLE_MAIN);
		n->asc = tex_metrics_asc(FONTROLE_MAIN);
		n->desc = tex_metrics_desc(FONTROLE_MAIN);
	}

	// script role glyphs (128-255)
	for (int i = 128; i < TEX_RESERVED_COUNT; i++)
	{
		Node* n = &g_reserved_nodes[i];
		n->type = N_GLYPH;
		n->flags = TEX_FLAG_SCRIPT;

		// back to ASCII (e.g., 128 -> 0) for the actual glyph code
		uint16_t ascii_code = (uint16_t)(i - 128);
		n->data.glyph = ascii_code;

		n->w = tex_metrics_glyph_width((unsigned int)ascii_code, FONTROLE_SCRIPT);
		n->asc = tex_metrics_asc(FONTROLE_SCRIPT);
		n->desc = tex_metrics_desc(FONTROLE_SCRIPT);
	}
}
