#ifndef TEX_TEX_MEASURE_H
#define TEX_TEX_MEASURE_H

#include "tex_internal.h"

typedef enum
{
	FONTROLE_MAIN = 0,
	FONTROLE_SCRIPT = 1
} FontRole;

// Measure nodes in range [start, end) linearly, deriving role from TEX_FLAG_SCRIPT
void tex_measure_range(UnifiedPool* pool, NodeRef start, NodeRef end);

#endif // TEX_TEX_MEASURE_H
