#include "cases.h"

static const TestCase g_basic_cases[] = {
	{ "fraction_ab", NULL, "$\\frac{a}{b}$", "84F078AC", 10, 5 },
	{ "sqrt_x", NULL, "$\\sqrt{x}$", "33739B7B", 10, 5 },
	{ "subscript_n", NULL, "$x_n$", "1F0E90C9", 10, 5 },
	{ "superscript_2", NULL, "$x^2$", "E0A4510D", 10, 5 },
	{ "sub_sup", NULL, "$x_i^2$", "E093AB53", 10, 5 },
	{ "fraction_poly", NULL, "$\\frac{x^2 + 1}{y_n}$", "3B4A61C2", 10, 5 }
};

static const TestCase g_complex_cases[] = {
	{ "quadratic", NULL, "$x = \\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}$", "025A9B2B", 10, 5 },
	{ "integral", NULL, "$\\int_{a}^{b} f(x) \\, dx$", "D1829FCF", 10, 5 },
	{ "sum_infty", NULL, "$\\sum_{n=0}^{\\infty} \\frac{x^n}{n!}$", "730F2145", 10, 5 },
	{ "limit", NULL, "$\\lim_{h \\to 0} \\frac{f(x+h) - f(x)}{h}$", "9D5B0D2F", 10, 5 },
	{ "integral_id", NULL, "$\\int_{a}^{b} f(x) \\, dx = F(b) - F(a)$", "750457D6", 10, 5 },
	{ "taylor_series", NULL, "$f(x) \\approx f(a) + f'(a)(x-a) + \\frac{f''(a)}{2}(x-a)^2$", "98A44073", 10, 5 },
	{ "maclaurin_exp", NULL, "$e^x = \\sum_{n=0}^{\\infty} \\frac{x^n}{n!}$", "28234AA9", 10, 5 },
	{ "maclaurin_sin", NULL, "$\\sin x = \\sum_{n=0}^{\\infty} \\frac{(-1)^n x^{2n+1}}{(2n+1)!}$", "0B91532B", 10, 5 },
	{ "normal_dist", NULL, "$P(x) = \\frac{1}{\\sigma \\sqrt{2\\pi}} e^{-\\frac{1}{2}\\left(\\frac{x-\\mu}{\\sigma}\\right)^2}$", "A1D31211", 10, 5 },
	{ "binom_coeff", NULL, "$$\\binom{n}{k} = \\frac{n!}{k!(n-k)!}$$", "D4CD94CC", 10, 5 }
};

static const TestCase g_matrix_cases[] = {
	{ "pmatrix_id", NULL, "$\\begin{pmatrix}1 & 0 \\\\ 0 & 1\\end{pmatrix}$", "DA4878FD", 10, 5 },
	{ "bmatrix_abcd", NULL, "$\\begin{bmatrix}a & b \\\\ c & d\\end{bmatrix}$", "B9A32D02", 10, 5 },
	{ "vmatrix_abcd", NULL, "$\\begin{vmatrix}a & b \\\\ c & d\\end{vmatrix}$", "0060BC19", 10, 5 },
	{ "bmatrix_sys", NULL, "$\\begin{Bmatrix}x + y = 5 \\\\ 2x - y = 1\\end{Bmatrix}$", "F0672F44", 10, 5 },
	{ "pmatrix_3x3", NULL, "$\\begin{pmatrix}1 & 0 & 0 \\\\ 0 & 1 & 0 \\\\ 0 & 0 & 1\\end{pmatrix}$", "FFECC82D", 10, 5 },
	{ "bmatrix_rot", NULL, "$\\begin{bmatrix}\\cos\\theta & -\\sin\\theta \\\\ \\sin\\theta & \\cos\\theta\\end{bmatrix}$", "6A6E67D4", 10, 5 },
	{ "vmatrix_3x3", NULL, "$\\begin{vmatrix}a & b & c \\\\ d & e & f \\\\ g & h & i\\end{vmatrix}$", "5E2EC5F1", 10, 5 },
	{ "array_augment", NULL, "$\\left[\\begin{array}{cc|c}1 & 2 & 3 \\\\ 4 & 5 & 6\\end{array}\\right]$", "548941E4", 10, 5 }
};

static const TestCase g_accents_cases[] = {
	{ "bar_x", NULL, "$\\bar{x}$", "16D313DF", 10, 5 },
	{ "hat_x", NULL, "$\\hat{x}$", "34AD701F", 10, 5 },
	{ "vec_v", NULL, "$\\vec{v}$", "A9F5A059", 10, 5 },
	{ "dot_x", NULL, "$\\dot{x}$", "FF2C7DAB", 10, 5 },
	{ "underbrace", NULL, "$\\underbrace{a + b + c}_{\\text{sum}}$", "9C71D53D", 10, 5 },
	{ "overline_ab", NULL, "$\\overline{AB}$", "69A43FFE", 10, 5 },
	{ "tilde_x", NULL, "$\\tilde{x}$", "4E71100E", 10, 5 },
	{ "ddot_x", NULL, "$\\ddot{x}$", "51238888", 10, 5 }
};

static const TestCase g_operators_cases[] = {
	{ "prod_limits", NULL, "$\\prod_{i=1}^{n} a_i$", "38903591", 10, 5 },
	{ "nested_frac", NULL, "$\\frac{1}{1 + \\frac{1}{2 + \\frac{1}{3}}}$", "049A5A15", 10, 5 },
	{ "sqrt_nested", NULL, "$\\sqrt{1 + \\sqrt{2 + \\sqrt{3}}}$", "6183A00C", 10, 5 }
};

const TestSuite g_test_suites[] = {
	{ "basic", g_basic_cases, sizeof(g_basic_cases) / sizeof(g_basic_cases[0]) },
	{ "complex", g_complex_cases, sizeof(g_complex_cases) / sizeof(g_complex_cases[0]) },
	{ "matrix", g_matrix_cases, sizeof(g_matrix_cases) / sizeof(g_matrix_cases[0]) },
	{ "accents", g_accents_cases, sizeof(g_accents_cases) / sizeof(g_accents_cases[0]) },
	{ "operators", g_operators_cases, sizeof(g_operators_cases) / sizeof(g_operators_cases[0]) }
};

const size_t g_test_suite_count = sizeof(g_test_suites) / sizeof(g_test_suites[0]);
