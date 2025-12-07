
#ifndef TEX_TEX_PARSE_H
#define TEX_TEX_PARSE_H

#include "tex_internal.h"

// Forward declaration (TeX_Layout is defined in tex_internal.h)

// Parse a math expression.
// Errors are reported via TEX_SET_ERROR on the layout.
// Returns the root N_MATH node on success, NULL on error.
Node* tex_parse_math(const char* input, int len, TeX_Layout* layout);

#endif // TEX_TEX_PARSE_H


