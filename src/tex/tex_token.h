
#ifndef TEX_TEX_TOKEN_H
#define TEX_TEX_TOKEN_H

#include <stddef.h>
#include "tex_arena.h"

// Forward declarations
struct TeX_Layout;

typedef enum
{
	T_TEXT = 0,
	T_SPACE,
	T_NEWLINE,
	T_MATH_INLINE,
	T_MATH_DISPLAY,
	T_LBRACE,
	T_RBRACE,
	T_CARET,
	T_UNDERSCORE,
	T_COMMAND,
	T_SYMBOL_CHAR,
	T_DOLLAR,
	T_EOF
} TokenType;

typedef struct
{
	TokenType type;
	const char* start;
	int len;
	int aux;
} TeX_Token;

// Streaming tokenizer state
typedef struct
{
	const char* cursor;
	const char* end;
} TeX_Stream;

// Initialize stream for tokenizing
void tex_stream_init(TeX_Stream* s, const char* input, int len);

// get next token from stream
// arena: optional arena for unescaped string allocation (NULL for dry-run)
// layout: for error reporting
// returns 1 if token produced, 0 on EOF
int tex_stream_next(TeX_Stream* s, TeX_Token* out, TexArena* arena, struct TeX_Layout* layout);

#endif // TEX_TEX_TOKEN_H
