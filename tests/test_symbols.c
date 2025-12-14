#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "include/texfont.h"
#include "src/tex/tex_symbols.h"

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void check_sym(const char* name, SymbolKind kind, int code)
{
	SymbolDesc d;
	memset(&d, 0, sizeof(d));
	int ok = texsym_find(name, strlen(name), &d);
	if (!(ok && d.kind == kind))
	{
		fprintf(stderr, "lookup failed: %s (ok=%d kind=%d)\n", name, ok, d.kind);
		assert(0);
	}
	if (kind == SYM_GLYPH)
	{
		int expect_u = (unsigned char)code;
		if ((int)d.code != expect_u)
		{
			fprintf(stderr, "code mismatch: %s -> got=%d expect=%d\n", name, (int)d.code, expect_u);
		}
		assert((int)d.code == expect_u);
	}
}

int main(void)
{
	// Glyphs: sample from each category
	check_sym("alpha", SYM_GLYPH, TEXFONT_alpha_CHAR);
	check_sym("Gamma", SYM_GLYPH, TEXFONT_GAMMA_CHAR);
	check_sym("partial", SYM_GLYPH, TEXFONT_PARTIAL_CHAR);
	check_sym("prime", SYM_GLYPH, TEXFONT_PRIME_CHAR);
	check_sym("int", SYM_GLYPH, TEXFONT_INTEGRAL_CHAR);
	check_sym("sum", SYM_GLYPH, TEXFONT_SUMMATION_CHAR);
	check_sym("prod", SYM_GLYPH, TEXFONT_PRODUCT_CHAR);
	check_sym("pm", SYM_GLYPH, TEXFONT_PLUS_MINUS_CHAR);
	check_sym("ge", SYM_GLYPH, TEXFONT_GREATER_EQUAL_CHAR);
	check_sym("to", SYM_GLYPH, TEXFONT_ARROW_RIGHT_CHAR);
	check_sym("gets", SYM_GLYPH, TEXFONT_ARROW_LEFT_CHAR);
	check_sym("langle", SYM_GLYPH, TEXFONT_LANGLE_CHAR);
	check_sym("rangle", SYM_GLYPH, TEXFONT_RANGLE_CHAR);

	// Structural
	SymbolDesc d;
	memset(&d, 0, sizeof(d));
	assert(texsym_find("frac", 4, &d) && d.kind == SYM_STRUCT);
	assert(texsym_find("sqrt", 4, &d) && d.kind == SYM_STRUCT);

	// Functions
	assert(texsym_find("sin", 3, &d) && d.kind == SYM_FUNC);
	assert(texsym_find("cos", 3, &d) && d.kind == SYM_FUNC);
	assert(texsym_find("tan", 3, &d) && d.kind == SYM_FUNC);
	assert(texsym_find("ln", 2, &d) && d.kind == SYM_FUNC);
	assert(texsym_find("lim", 3, &d) && d.kind == SYM_FUNC);

	// Accents
	assert(texsym_find("vec", 3, &d) && d.kind == SYM_ACCENT);
	assert(texsym_find("hat", 3, &d) && d.kind == SYM_ACCENT);
	assert(texsym_find("bar", 3, &d) && d.kind == SYM_ACCENT);
	assert(texsym_find("dot", 3, &d) && d.kind == SYM_ACCENT);

	// Spaces
	assert(texsym_find(",", 1, &d) && d.kind == SYM_SPACE); // thin space
	assert(texsym_find(":", 1, &d) && d.kind == SYM_SPACE); // med space
	assert(texsym_find(";", 1, &d) && d.kind == SYM_SPACE); // thick space
	assert(texsym_find("!", 1, &d) && d.kind == SYM_SPACE); // negative thin
	assert(texsym_find("quad", (int)strlen("quad"), &d) && d.kind == SYM_SPACE); // em space
	int ok_qquad = texsym_find("qquad", (int)strlen("qquad"), &d);
	if (!(ok_qquad && d.kind == SYM_SPACE))
	{
		fprintf(stderr, "qquad lookup: ok=%d kind=%d\n", ok_qquad, d.kind);
	}
	assert(ok_qquad && d.kind == SYM_SPACE); // 2em space

	// Unknown
	assert(!texsym_find("does_not_exist", 14, &d));

	printf("test_symbols: PASS\n");
	return 0;
}
