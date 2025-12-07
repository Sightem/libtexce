#ifndef TEX_TEX_MEASURE_H
#define TEX_TEX_MEASURE_H

#include "tex_internal.h"

typedef enum
{
	FONTROLE_MAIN = 0,
	FONTROLE_SCRIPT = 1
} FontRole;

void tex_measure_node(Node* n, FontRole role);

#endif // TEX_TEX_MEASURE_H
