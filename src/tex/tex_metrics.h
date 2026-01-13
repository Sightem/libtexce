#ifndef TEX_TEX_METRICS_H
#define TEX_TEX_METRICS_H

#include "tex.h"
#include "tex_measure.h"
#include "texfont.h"

static inline int tex_is_big_operator(unsigned int glyph)
{
	unsigned char g = (unsigned char)glyph;
	return g == (unsigned char)TEXFONT_INTEGRAL_CHAR || g == (unsigned char)TEXFONT_SUMMATION_CHAR ||
	    g == (unsigned char)TEXFONT_PRODUCT_CHAR;
}

// helper that treats multioperator nodes
static inline int tex_node_is_big_operator(const Node* node)
{
	if (!node)
		return 0;
	if (node->type == N_MULTIOP)
		return 1;
	if (node->type == N_GLYPH)
		return tex_is_big_operator(node->data.glyph);
	return 0;
}

// forward declaration
struct TeX_Layout;

// initialize metrics using layout
void tex_metrics_init(struct TeX_Layout* layout);

// explicit reset to host constants (used by tests/host builds implicitly)
void tex_metrics_reset(void);

int16_t tex_metrics_math_axis(void);

// query per role metrics
int16_t tex_metrics_asc(FontRole role);
int16_t tex_metrics_desc(FontRole role);

// measure text/glyph width for the given role
int16_t tex_metrics_text_width(const char* s, FontRole role);
int16_t tex_metrics_text_width_n(const char* s, int len, FontRole role);
int16_t tex_metrics_glyph_width(unsigned int glyph, FontRole role);

extern FontRole g_tex_metrics_current_role;
static inline void tex_metrics_invalidate_font_state(void) { g_tex_metrics_current_role = (FontRole)-1; }

void tex_reserved_init(void);

#endif // TEX_TEX_METRICS_H
