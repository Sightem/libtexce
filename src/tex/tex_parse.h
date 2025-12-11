
#ifndef TEX_TEX_PARSE_H
#define TEX_TEX_PARSE_H

#include "tex_internal.h"

// Parse a math expression.
// pool: UnifiedPool for node and string allocation
// layout: for error reporting
// errors are reported via TEX_SET_ERROR on the layout.
// returns the root N_MATH node ref on success, NODE_NULL on error.
NodeRef tex_parse_math(const char* input, int len, UnifiedPool* pool, TeX_Layout* layout);

#endif // TEX_TEX_PARSE_H
