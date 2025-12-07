
#ifndef TEX_TEX_TOKEN_H
#define TEX_TEX_TOKEN_H

// Forward declaration
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

// Tokenize top level input
//
// two pass design:
//   pass 1: out_tokens == NULL -> returns required token count
//   pass 2: out_tokens != NULL -> fills tokens up to max_tokens
//
// errors are reported via TEX_SET_ERROR on the layout. on error, returns -1
// and the caller should check tex_get_last_error(layout) for details
//
// returns:
//   >= 0 : number of tokens (including T_EOF)
//   -1   : error occurred (check layout for details)
int tex_tokenize_top_level(const char* input, TeX_Token* out_tokens, int max_tokens, struct TeX_Layout* layout);

#endif // TEX_TEX_TOKEN_H
