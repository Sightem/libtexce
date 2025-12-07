#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex.h"

int main(void)
{
	const char* text = "Here is a line with inline math $a^2 + b^2 = c^2$ and some more text to wrap.\n"
	                   "Next line with a fraction: $ \\frac{1}{1+\\frac{1}{x}} $.\n"
	                   "And a display block: $$ \\int_0^1 x^2 \\; dx $$ followed by text.\n";

	char* buf = (char*)malloc(strlen(text) + 1);
	strcpy(buf, text);

	TeX_Config cfg = { 0 };
	cfg.color_fg = 0;
	cfg.color_bg = 255;
	cfg.font_pack = NULL; // host path: metrics use fallback

	TeX_Layout* L = tex_format(buf, 320, &cfg);
	if (!L)
	{
		fprintf(stderr, "tex_format returned NULL\n");
		return 1;
	}

	// Trigger a first draw to log draw stats (host recorder mode does not render)
	tex_draw(L, 0, 0, 0);

	tex_free(L);
	free(buf);
	return 0;
}
