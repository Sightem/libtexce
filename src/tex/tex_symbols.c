#include "tex_symbols.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "texfont.h"

typedef struct
{
	const char* name;
	uint16_t code;
	SymbolKind kind;
} MapEnt;

#define GL(CC) ((uint16_t)(unsigned char)(CC))

// sorted lexicographically by name
static const MapEnt g_map[] = {
	{ "!", SYMC_NEGSPACE, SYM_SPACE },
	{ ",", SYMC_THINSPACE, SYM_SPACE },
	{ ":", SYMC_MEDSPACE, SYM_SPACE },
	{ ";", SYMC_THICKSPACE, SYM_SPACE },
	{ "Delta", GL(TEXFONT_DELTA_CHAR), SYM_GLYPH },
	{ "Gamma", GL(TEXFONT_GAMMA_CHAR), SYM_GLYPH },
	{ "Lambda", GL(TEXFONT_LAMBDA_CHAR), SYM_GLYPH },
	{ "Omega", GL(TEXFONT_OMEGA_CHAR), SYM_GLYPH },
	{ "Phi", GL(TEXFONT_PHI_CHAR), SYM_GLYPH },
	{ "Pi", GL(TEXFONT_PI_CHAR), SYM_GLYPH },
	{ "Pr", SYMC_FUNC_PR, SYM_FUNC },
	{ "Psi", GL(TEXFONT_PSI_CHAR), SYM_GLYPH },
	{ "Sigma", GL(TEXFONT_SIGMA_CHAR), SYM_GLYPH },
	{ "Theta", GL(TEXFONT_THETA_CHAR), SYM_GLYPH },
	{ "Xi", GL(TEXFONT_XI_CHAR), SYM_GLYPH },
	{ "alpha", GL(TEXFONT_alpha_CHAR), SYM_GLYPH },
	{ "angle", GL(TEXFONT_ANGLE_CHAR), SYM_GLYPH },
	{ "approx", GL(TEXFONT_APPROX_CHAR), SYM_GLYPH },
	{ "arccos", SYMC_FUNC_ARCCOS, SYM_FUNC },
	{ "arcsin", SYMC_FUNC_ARCSIN, SYM_FUNC },
	{ "arctan", SYMC_FUNC_ARCTAN, SYM_FUNC },
	{ "arg", SYMC_FUNC_ARG, SYM_FUNC },
	{ "ast", GL('*'), SYM_GLYPH },
	{ "bar", SYMC_ACC_BAR, SYM_ACCENT },
	{ "beta", GL(TEXFONT_beta_CHAR), SYM_GLYPH },
	{ "cap", GL(TEXFONT_INTERSECTION_CHAR), SYM_GLYPH },
	{ "cdot", GL(TEXFONT_DOT_OP_CHAR), SYM_GLYPH },
	{ "chi", GL(TEXFONT_chi_CHAR), SYM_GLYPH },
	{ "circ", GL(TEXFONT_RING_OP_CHAR), SYM_GLYPH },
	{ "cong", GL(TEXFONT_CONGRUENT_CHAR), SYM_GLYPH },
	{ "cos", SYMC_FUNC_COS, SYM_FUNC },
	{ "cosh", SYMC_FUNC_COSH, SYM_FUNC },
	{ "cot", SYMC_FUNC_COT, SYM_FUNC },
	{ "coth", SYMC_FUNC_COTH, SYM_FUNC },
	{ "csc", SYMC_FUNC_CSC, SYM_FUNC },
	{ "cup", GL(TEXFONT_UNION_CHAR), SYM_GLYPH },
	{ "ddot", SYMC_ACC_DDOT, SYM_ACCENT },
	{ "deg", SYMC_FUNC_DEG, SYM_FUNC },
	{ "degree", GL(TEXFONT_DEGREE_CHAR), SYM_GLYPH },
	{ "delta", GL(TEXFONT_delta_CHAR), SYM_GLYPH },
	{ "det", SYMC_FUNC_DET, SYM_FUNC },
	{ "dim", SYMC_FUNC_DIM, SYM_FUNC },
	{ "div", GL(TEXFONT_DIVIDE_CHAR), SYM_GLYPH },
	{ "dot", SYMC_ACC_DOT, SYM_ACCENT },
	{ "ell", GL(TEXFONT_ELL_CHAR), SYM_GLYPH },
	{ "emptyset", GL(TEXFONT_EMPTY_SET_CHAR), SYM_GLYPH },
	{ "epsilon", GL(TEXFONT_epsilon_CHAR), SYM_GLYPH },
	{ "equiv", GL(TEXFONT_EQUIVALENT_CHAR), SYM_GLYPH },
	{ "eta", GL(TEXFONT_eta_CHAR), SYM_GLYPH },
	{ "exists", GL(TEXFONT_EXISTS_CHAR), SYM_GLYPH },
	{ "exp", SYMC_FUNC_EXP, SYM_FUNC },
	{ "forall", GL(TEXFONT_FOR_ALL_CHAR), SYM_GLYPH },
	{ "frac", SYMC_FRAC, SYM_STRUCT },
	{ "gamma", GL(TEXFONT_gamma_CHAR), SYM_GLYPH },
	{ "gcd", SYMC_FUNC_GCD, SYM_FUNC },
	{ "ge", GL(TEXFONT_GREATER_EQUAL_CHAR), SYM_GLYPH },
	{ "geq", GL(TEXFONT_GREATER_EQUAL_CHAR), SYM_GLYPH },
	{ "gets", GL(TEXFONT_ARROW_LEFT_CHAR), SYM_GLYPH },
	{ "hat", SYMC_ACC_HAT, SYM_ACCENT },
	{ "hbar", GL(TEXFONT_HBAR_CHAR), SYM_GLYPH },
	{ "hom", SYMC_FUNC_HOM, SYM_FUNC },
	{ "iiiint", SYMC_MULTIINT_4, SYM_MULTIOP },
	{ "iiint", SYMC_MULTIINT_3, SYM_MULTIOP },
	{ "iint", SYMC_MULTIINT_2, SYM_MULTIOP },
	{ "in", GL(TEXFONT_ELEMENT_OF_CHAR), SYM_GLYPH },
	{ "inf", SYMC_FUNC_INF, SYM_FUNC },
	{ "infty", GL(TEXFONT_INFINITY_CHAR), SYM_GLYPH },
	{ "int", GL(TEXFONT_INTEGRAL_CHAR), SYM_GLYPH },
	{ "iota", GL(TEXFONT_iota_CHAR), SYM_GLYPH },
	{ "kappa", GL(TEXFONT_kappa_CHAR), SYM_GLYPH },
	{ "ker", SYMC_FUNC_KER, SYM_FUNC },
	{ "lambda", GL(TEXFONT_lambda_CHAR), SYM_GLYPH },
	{ "langle", GL(TEXFONT_LANGLE_CHAR), SYM_GLYPH },
	{ "lbrace", GL('{'), SYM_GLYPH },
	{ "lceil", GL('['), SYM_GLYPH },
	{ "le", GL(TEXFONT_LESS_EQUAL_CHAR), SYM_GLYPH },
	{ "left", 0, SYM_DELIM_MOD },
	{ "leftarrow", GL(TEXFONT_ARROW_LEFT_CHAR), SYM_GLYPH },
	{ "leq", GL(TEXFONT_LESS_EQUAL_CHAR), SYM_GLYPH },
	{ "lfloor", GL('['), SYM_GLYPH },
	{ "lg", SYMC_FUNC_LG, SYM_FUNC },
	{ "lim", SYMC_FUNC_LIM, SYM_FUNC },
	{ "ln", SYMC_FUNC_LN, SYM_FUNC },
	{ "log", SYMC_FUNC_LOG, SYM_FUNC },
	{ "max", SYMC_FUNC_MAX, SYM_FUNC },
	{ "min", SYMC_FUNC_MIN, SYM_FUNC },
	{ "mp", GL(TEXFONT_MINUS_PLUS_CHAR), SYM_GLYPH },
	{ "mu", GL(TEXFONT_mu_CHAR), SYM_GLYPH },
	{ "nabla", GL(TEXFONT_NABLA_CHAR), SYM_GLYPH },
	{ "ne", GL(TEXFONT_NOT_EQUAL_CHAR), SYM_GLYPH },
	{ "neq", GL(TEXFONT_NOT_EQUAL_CHAR), SYM_GLYPH },
	{ "notin", GL(TEXFONT_NOT_IN_CHAR), SYM_GLYPH },
	{ "nu", GL(TEXFONT_nu_CHAR), SYM_GLYPH },
	{ "oiiint", SYMC_MULTI_OINT_3, SYM_MULTIOP },
	{ "oiint", SYMC_MULTI_OINT_2, SYM_MULTIOP },
	{ "oint", SYMC_MULTI_OINT_1, SYM_MULTIOP },
	{ "omega", GL(TEXFONT_omega_CHAR), SYM_GLYPH },
	{ "omicron", GL(TEXFONT_omicron_CHAR), SYM_GLYPH },
	{ "oplus", GL(TEXFONT_O_PLUS_CHAR), SYM_GLYPH },
	{ "overbrace", SYMC_OVERBRACE, SYM_STRUCT },
	{ "overline", SYMC_ACC_OVERLINE, SYM_ACCENT },
	{ "parallel", GL(TEXFONT_PARALLEL_CHAR), SYM_GLYPH },
	{ "partial", GL(TEXFONT_PARTIAL_CHAR), SYM_GLYPH },
	{ "perp", GL(TEXFONT_PERPENDICULAR_CHAR), SYM_GLYPH },
	{ "phi", GL(TEXFONT_phi_CHAR), SYM_GLYPH },
	{ "pi", GL(TEXFONT_pi_CHAR), SYM_GLYPH },
	{ "pm", GL(TEXFONT_PLUS_MINUS_CHAR), SYM_GLYPH },
	{ "prime", GL(TEXFONT_PRIME_CHAR), SYM_GLYPH },
	{ "prod", GL(TEXFONT_PRODUCT_CHAR), SYM_GLYPH },
	{ "propto", GL(TEXFONT_PROPORTIONAL_CHAR), SYM_GLYPH },
	{ "psi", GL(TEXFONT_psi_CHAR), SYM_GLYPH },
	{ "qquad", SYMC_QQUAD, SYM_SPACE },
	{ "quad", SYMC_QUAD, SYM_SPACE },
	{ "rangle", GL(TEXFONT_RANGLE_CHAR), SYM_GLYPH },
	{ "rbrace", GL('}'), SYM_GLYPH },
	{ "rceil", GL(']'), SYM_GLYPH },
	{ "rfloor", GL(']'), SYM_GLYPH },
	{ "rho", GL(TEXFONT_rho_CHAR), SYM_GLYPH },
	{ "right", 0, SYM_DELIM_MOD },
	{ "rightarrow", GL(TEXFONT_ARROW_RIGHT_CHAR), SYM_GLYPH },
	{ "sec", SYMC_FUNC_SEC, SYM_FUNC },
	{ "sigma", GL(TEXFONT_sigma_CHAR), SYM_GLYPH },
	{ "sim", GL(TEXFONT_SIMILAR_CHAR), SYM_GLYPH },
	{ "sin", SYMC_FUNC_SIN, SYM_FUNC },
	{ "sinh", SYMC_FUNC_SINH, SYM_FUNC },
	{ "sqrt", SYMC_SQRT, SYM_STRUCT },
	{ "subset", GL(TEXFONT_SUBSET_OF_CHAR), SYM_GLYPH },
	{ "subseteq", GL(TEXFONT_SUBSET_EQ_CHAR), SYM_GLYPH },
	{ "sum", GL(TEXFONT_SUMMATION_CHAR), SYM_GLYPH },
	{ "sup", SYMC_FUNC_SUP, SYM_FUNC },
	{ "tan", SYMC_FUNC_TAN, SYM_FUNC },
	{ "tanh", SYMC_FUNC_TANH, SYM_FUNC },
	{ "tau", GL(TEXFONT_tau_CHAR), SYM_GLYPH },
	{ "text", SYMC_TEXT, SYM_STRUCT },
	{ "tfrac", SYMC_FRAC, SYM_STRUCT },
	{ "therefore", GL(TEXFONT_THEREFORE_CHAR), SYM_GLYPH },
	{ "theta", GL(TEXFONT_theta_CHAR), SYM_GLYPH },
	{ "times", GL(TEXFONT_TIMES_CHAR), SYM_GLYPH },
	{ "to", GL(TEXFONT_ARROW_RIGHT_CHAR), SYM_GLYPH },
	{ "underbrace", SYMC_UNDERBRACE, SYM_STRUCT },
	{ "underline", SYMC_ACC_UNDERLINE, SYM_ACCENT },
	{ "upsilon", GL(TEXFONT_upsilon_CHAR), SYM_GLYPH },
	{ "vec", SYMC_ACC_VEC, SYM_ACCENT },
	{ "xi", GL(TEXFONT_xi_CHAR), SYM_GLYPH },
	{ "zeta", GL(TEXFONT_zeta_CHAR), SYM_GLYPH },
};


static int name_cmp(const char* a, size_t alen, const char* b)
{
	size_t blen = strlen(b);
	size_t m = (alen < blen) ? alen : blen;
	int c = strncmp(a, b, m);
	if (c)
		return c;
	if (alen == blen)
		return 0;
	return (alen < blen) ? -1 : 1;
}

typedef struct
{
	const char* s;
	size_t len;
} Key;
static int cmp_key_ent(const void* kptr, const void* eptr)
{
	const Key* k = (const Key*)kptr;
	const MapEnt* e = (const MapEnt*)eptr;
	return name_cmp(k->s, k->len, e->name);
}

int texsym_find(const char* s, size_t len, SymbolDesc* out)
{
	if (out)
	{
		out->name = NULL;
		out->code = 0;
		out->kind = SYM_NONE;
	}
	if (!s || len == 0)
		return 0;
	size_t n = sizeof(g_map) / sizeof(g_map[0]);
	Key key = { s, len };
	const MapEnt* p = (const MapEnt*)bsearch(&key, g_map, n, sizeof(g_map[0]), cmp_key_ent);
	if (!p)
		return 0;
	if (out)
	{
		out->name = p->name;
		out->code = p->code;
		out->kind = p->kind;
	}
	return 1;
}
