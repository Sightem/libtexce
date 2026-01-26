#include "app.h"
#include "steps.h"

#include <string.h>

static void steps_menu_build_cramer(App* app)
{
	app->steps_menu.selected = 0;
	app->steps_menu.count = 0;
	for (uint8_t i = 0; i < (uint8_t)(sizeof(app->steps_menu.items) / sizeof(app->steps_menu.items[0])); ++i)
		app->steps_menu.items[i] = nullptr;

	// 0: Overview
	strncpy(app->steps_menu.labels[0], "Overview", sizeof(app->steps_menu.labels[0]));
	app->steps_menu.labels[0][sizeof(app->steps_menu.labels[0]) - 1] = '\0';
	app->steps_menu.items[0] = app->steps_menu.labels[0];

	// 1: Delta
	strncpy(app->steps_menu.labels[1], "Delta", sizeof(app->steps_menu.labels[1]));
	app->steps_menu.labels[1][sizeof(app->steps_menu.labels[1]) - 1] = '\0';
	app->steps_menu.items[1] = app->steps_menu.labels[1];

	const uint8_t a_slot = app->last_op.lhs_slot;
	uint8_t n = 0;
	if (a_slot < MATRIX_SLOTS && matrix_is_set(&app->matrices[a_slot]))
		n = app->matrices[a_slot].rows;
	if (n > 6)
		n = 6;

	// Delta_i entries (cap at remaining slots)
	const uint8_t max_items = (uint8_t)(sizeof(app->steps_menu.items) / sizeof(app->steps_menu.items[0]));
	uint8_t count = 2;
	for (uint8_t i = 0; i < n && count < max_items; ++i)
	{
		char* dst = app->steps_menu.labels[count];
		dst[0] = 'D';
		dst[1] = 'e';
		dst[2] = 'l';
		dst[3] = 't';
		dst[4] = 'a';
		// 1-based index, n<=6 so single digit
		dst[5] = (char)('1' + i);
		dst[6] = '\0';
		app->steps_menu.items[count] = dst;
		++count;
	}
	app->steps_menu.count = count;
}

static void generate_cramer_overview_steps(App* app)
{
	const uint8_t a_slot = app->last_op.lhs_slot;
	const uint8_t b_slot = app->last_op.rhs_slot;

	steps_clear(&app->last_steps);
	(void)steps_begin(&app->last_steps, OP_CRAMER);

	if (a_slot < MATRIX_SLOTS)
		(void)steps_append_matrix(&app->last_steps, "A", &app->matrices[a_slot]);
	if (b_slot < MATRIX_SLOTS)
		(void)steps_append_matrix(&app->last_steps, "b", &app->matrices[b_slot]);

	(void)steps_append_tex(&app->last_steps, "Cramer's rule",
	                       "\\Delta=\\det(A),\\ \\Delta_i=\\det(A_i),\\ x_i=\\frac{\\Delta_i}{\\Delta}");
	if (app->has_last_result)
		(void)steps_append_matrix(&app->last_steps, "x", &app->last_result);
}

static void generate_cramer_delta_steps(App* app, uint8_t which_delta, bool is_delta_i)
{
	const uint8_t a_slot = app->last_op.lhs_slot;
	const uint8_t b_slot = app->last_op.rhs_slot;
	if (a_slot >= MATRIX_SLOTS || b_slot >= MATRIX_SLOTS)
		return;

	const MatrixSlot* a = &app->matrices[a_slot];
	const MatrixSlot* b = &app->matrices[b_slot];
	if (!matrix_is_set(a) || !matrix_is_set(b))
		return;
	if (a->rows != a->cols || b->cols != 1 || b->rows != a->rows)
		return;

	steps_clear(&app->last_steps);
	(void)steps_begin(&app->last_steps, OP_CRAMER);

	MatrixSlot tmp = *a;
	if (is_delta_i)
	{
		const uint8_t col = which_delta;
		if (col >= a->cols)
			return;
		for (uint8_t r = 0; r < a->rows; ++r)
			tmp.cell[r][col] = b->cell[r][0];
	}

	if (!is_delta_i)
		(void)steps_append_tex(&app->last_steps, "Goal", "\\Delta=\\det(A)");
	else
		(void)steps_append_tex(&app->last_steps, "Goal", "\\Delta_i=\\det(A_i)");

	Rational det = Rational{0, 1};
	(void)matrix_det_steps(&tmp, &det, &app->last_steps);

	// final equation
	if (!is_delta_i)
	{
		(void)steps_append_tex(&app->last_steps, "Result", "\\Delta=\\det(A)");
	}
	else
	{
		char eq[48];
		eq[0] = '\\';
		eq[1] = 'D';
		eq[2] = 'e';
		eq[3] = 'l';
		eq[4] = 't';
		eq[5] = 'a';
		eq[6] = '_';
		eq[7] = (char)('1' + which_delta);
		eq[8] = '=';
		eq[9] = '\\';
		eq[10] = 'd';
		eq[11] = 'e';
		eq[12] = 't';
		eq[13] = '(';
		eq[14] = 'A';
		eq[15] = '_';
		eq[16] = (char)('1' + which_delta);
		eq[17] = ')';
		eq[18] = '\0';
		(void)steps_append_tex(&app->last_steps, "Result", eq);
	}

	// append numeric value as 1x1 matrix for now
	MatrixSlot detm;
	matrix_clear(&detm);
	detm.rows = 1;
	detm.cols = 1;
	detm.cell[0][0] = det;
	(void)steps_append_matrix(&app->last_steps, "Value", &detm);
}

void screen_enter_steps_menu(App* app)
{
	if (app->last_op.op == OP_CRAMER)
		steps_menu_build_cramer(app);
	else
	{
		app->steps_menu.selected = 0;
		app->steps_menu.count = 1;
		strncpy(app->steps_menu.labels[0], "Steps", sizeof(app->steps_menu.labels[0]));
		app->steps_menu.labels[0][sizeof(app->steps_menu.labels[0]) - 1] = '\0';
		app->steps_menu.items[0] = app->steps_menu.labels[0];
	}
}

void screen_exit_steps_menu(App* app) { (void)app; }

void screen_update_steps_menu(App* app, uint8_t key)
{
	UiMenu menu = {
		.title = "Steps",
		.items = app->steps_menu.items,
		.count = app->steps_menu.count,
		.selected = &app->steps_menu.selected,
	};

	bool back = false;
	const bool activated = ui_menu_update(&menu, key, &back);
	if (back)
	{
		nav_pop(app);
		return;
	}

	if (!activated)
		return;

	if (app->last_op.op != OP_CRAMER)
	{
		nav_push(app, SCREEN_STEPS_VIEW);
		return;
	}

	const uint8_t sel = app->steps_menu.selected;
	if (sel == 0)
	{
		generate_cramer_overview_steps(app);
		nav_push(app, SCREEN_STEPS_VIEW);
		return;
	}

	if (sel == 1)
	{
		generate_cramer_delta_steps(app, 0, false);
		nav_push(app, SCREEN_STEPS_VIEW);
		return;
	}

	const uint8_t which = (uint8_t)(sel - 2);
	generate_cramer_delta_steps(app, which, true);
	nav_push(app, SCREEN_STEPS_VIEW);
}

void screen_draw_steps_menu(App* app)
{
	UiMenu menu = {
		.title = "Steps",
		.items = app->steps_menu.items,
		.count = app->steps_menu.count,
		.selected = &app->steps_menu.selected,
	};
	ui_menu_draw(&menu);
}
