#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex/tex_token.h"
#include "tex/tex_arena.h"
#include "tex/tex_internal.h"

static int tests_run = 0, tests_failed = 0;

static void assert_true(int cond, const char* msg)
{
	++tests_run;
	if (!cond)
	{
		++tests_failed;
		fprintf(stderr, "FAIL: %s\n", msg);
	}
}

static int collect_tokens(char* buf, TeX_Token** out)
{
	TeX_Layout L; memset(&L, 0, sizeof(L));
	arena_init(&L.arena);
	int need = tex_tokenize_top_level(buf, NULL, 0, &L);
    if (need <= 0)
    {
        return need;
    }
	TeX_Token* arr = (TeX_Token*)calloc((size_t)need, sizeof(TeX_Token));
    int got = tex_tokenize_top_level(buf, arr, need, &L);
	*out = arr;
	return got;
}

static int str_eq_token(const TeX_Token* t, const char* s)
{
	return (int)strlen(s) == t->len && strncmp(t->start, s, (size_t)t->len) == 0;
}

int main(void)
{
	// 1) "$x$" -> T_MATH_INLINE("x")
	{
		char buf[] = "$x$";
		TeX_Token* toks = NULL;
		int n = collect_tokens(buf, &toks);
		assert_true(n >= 2, "math inline token count >= 2 (incl EOF)");
		assert_true(toks[0].type == T_MATH_INLINE, "inline math token type");
		assert_true(str_eq_token(&toks[0], "x"), "inline math payload 'x'");
		assert_true(toks[n - 1].type == T_EOF, "EOF terminator present");
		free(toks);
	}

	// 2) "$$x^2$$ text" -> T_MATH_DISPLAY("x^2"), T_SPACE, T_TEXT("text")
	{
		char buf[] = "$$x^2$$ text";
		TeX_Token* toks = NULL;
		int n = collect_tokens(buf, &toks);
		assert_true(n >= 4, "display math token count >= 4");
		assert_true(toks[0].type == T_MATH_DISPLAY, "display math token type");
		assert_true(str_eq_token(&toks[0], "x^2"), "display math payload 'x^2'");
		assert_true(toks[1].type == T_SPACE, "space after display math");
		assert_true(toks[2].type == T_TEXT && str_eq_token(&toks[2], "text"), "following text token");
		free(toks);
	}

	// 3) "a b\nc" -> T_TEXT("a"), T_SPACE, T_TEXT("b"), T_NEWLINE, T_TEXT("c")
	{
		char buf[] = "a b\nc";
		TeX_Token* toks = NULL;
		int n = collect_tokens(buf, &toks);
		assert_true(n >= 6, "text+space+newline token count >=6");
		assert_true(toks[0].type == T_TEXT && str_eq_token(&toks[0], "a"), "first word 'a'");
		assert_true(toks[1].type == T_SPACE, "space token");
		assert_true(toks[2].type == T_TEXT && str_eq_token(&toks[2], "b"), "second word 'b'");
		assert_true(toks[3].type == T_NEWLINE, "newline token");
		assert_true(toks[4].type == T_TEXT && str_eq_token(&toks[4], "c"), "third word 'c'");
		free(toks);
	}

	// 4) "$unclosed" -> T_TEXT("$unclosed")
	{
		char buf[] = "$unclosed";
		TeX_Token* toks = NULL;
		int n = collect_tokens(buf, &toks);
		assert_true(n >= 2, "unclosed math fallback count >=2");
		assert_true(toks[0].type == T_TEXT && str_eq_token(&toks[0], "$unclosed"), "fallback as single text token");
		free(toks);
	}

	// 5) Escapes inside math: "$\\$\\{\\}$" -> payload "${}"
	{
		char buf[] = "${\\$\\{\\}}"; // This is actually not delimited; correct to: "$\\$\\{\\}$"
		// Fix buffer to the intended test string (C string with escapes)
		strcpy(buf, "$\\$\\{\\}$");
		TeX_Token* toks = NULL;
		int n = collect_tokens(buf, &toks);
		assert_true(n >= 2, "escaped payload token count >=2");
		assert_true(toks[0].type == T_MATH_INLINE, "inline math token for escapes");
		assert_true(str_eq_token(&toks[0], "${}"), "payload unescaped to '${}'");
		free(toks);
	}

	// 6) Escapes in text: "price\\$100" -> T_TEXT("price$100")
	{
		char buf[] = "price\\$100";
		TeX_Token* toks = NULL;
		int n = collect_tokens(buf, &toks);
		assert_true(n >= 2, "text escape token count >=2");
		assert_true(toks[0].type == T_TEXT && str_eq_token(&toks[0], "price$100"), "text escape $ handled");
		free(toks);
	}

	if (tests_failed == 0)
	{
		printf("test_token: PASS (%d tests)\n", tests_run);
		return 0;
	}
	printf("test_token: FAIL (%d/%d failed)\n", tests_failed, tests_run);
	return 1;
}
