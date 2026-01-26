#include "app.h"
#include "steps.h"

#include <string.h>

static const char* const kItems[] = {
	"Minor M_ij",
	"Cofactor C_ij",
	"Both",
};

void screen_enter_element_output(App* app) { (void)app; }
void screen_exit_element_output(App* app) { (void)app; }

static void set_toast(App* app, const char* msg)
{
	app->has_toast = true;
	strncpy(app->toast, msg, sizeof(app->toast));
	app->toast[sizeof(app->toast) - 1] = '\0';
}

static void append_1x1_step(StepsLog* steps, const char* caption, Rational v)
{
	MatrixSlot m;
	matrix_clear(&m);
	m.rows = 1;
	m.cols = 1;
	m.cell[0][0] = v;
	(void)steps_append_matrix(steps, caption, &m);
}

static void compute_element_result(App* app)
{
	const uint8_t slot = app->element_query.slot;
	const uint8_t n = app->element_query.n;
	const uint8_t i = app->element_query.i;
	const uint8_t j = app->element_query.j;

	const MatrixSlot* a = &app->matrices[slot];
	if (!matrix_is_set(a) || a->rows != a->cols || a->rows != n || i < 1 || j < 1 || i > n || j > n)
	{
		steps_clear(&app->last_steps);
		set_toast(app, "Invalid selection");
		return;
	}

	Rational minor = Rational{1, 1};
	MatrixSlot sub;
	matrix_clear(&sub);
	if (n > 1)
	{
		sub.rows = (uint8_t)(n - 1);
		sub.cols = (uint8_t)(n - 1);

		uint8_t rr = 0;
		for (uint8_t r = 0; r < n; ++r)
		{
			if (r == (uint8_t)(i - 1))
				continue;
			uint8_t cc = 0;
			for (uint8_t c = 0; c < n; ++c)
			{
				if (c == (uint8_t)(j - 1))
					continue;
				sub.cell[rr][cc] = a->cell[r][c];
				++cc;
			}
			++rr;
		}
	}

	Rational cofactor = (((i + j) & 1u) == 0u) ? minor : rational_neg(minor);

	// build steps (lossy, overwritten)
	steps_clear(&app->last_steps);
	(void)steps_begin(&app->last_steps, OP_ELEMENT);
	app->last_op.op = OP_ELEMENT;
	app->last_op.lhs_slot = slot;
	app->last_op.rhs_slot = 0;
	app->last_op.i = i;
	app->last_op.j = j;

	(void)steps_append_matrix(&app->last_steps, "A", a);
	{
		char msg[48];
		// delete row i, col j
		msg[0] = 'D';
		msg[1] = 'e';
		msg[2] = 'l';
		msg[3] = 'e';
		msg[4] = 't';
		msg[5] = 'e';
		msg[6] = ' ';
		msg[7] = 'r';
		msg[8] = 'o';
		msg[9] = 'w';
		msg[10] = ' ';
		msg[11] = (char)('0' + i);
		msg[12] = ',';
		msg[13] = ' ';
		msg[14] = 'c';
		msg[15] = 'o';
		msg[16] = 'l';
		msg[17] = ' ';
		msg[18] = (char)('0' + j);
		msg[19] = '\0';
		(void)steps_append_tex(&app->last_steps, "Form submatrix", "A_{(i,j)}");
		(void)steps_append_tex(&app->last_steps, msg, "M_{ij}=\\det\\left(A_{(i,j)}\\right)");
	}

	if (n > 1)
	{
		if (!matrix_det_steps(&sub, &minor, &app->last_steps))
		{
			steps_clear(&app->last_steps);
			set_toast(app, "Det failed");
			return;
		}
	}
	else
	{
		(void)steps_append_tex(&app->last_steps, "Base case", "M_{11}=1");
	}

	append_1x1_step(&app->last_steps, "M_ij", minor);

	cofactor = (((i + j) & 1u) == 0u) ? minor : rational_neg(minor);
	if (app->element_query.mode != ELEM_OUT_MINOR)
	{
		(void)steps_append_tex(&app->last_steps, "Cofactor", "C_{ij}=(-1)^{i+j}M_{ij}");
		append_1x1_step(&app->last_steps, "C_ij", cofactor);
	}

	matrix_clear(&app->last_result);
	app->has_last_result = true;
	app->has_last_label = true;

	switch (app->element_query.mode)
	{
		case ELEM_OUT_MINOR:
			app->last_result.rows = 1;
			app->last_result.cols = 1;
			app->last_result.cell[0][0] = minor;
			app->has_last_label = true;
			app->last_label[0] = 'M';
			app->last_label[1] = (char)('0' + i);
			app->last_label[2] = (char)('0' + j);
			app->last_label[3] = ' ';
			app->last_label[4] = slot_name(slot);
			app->last_label[5] = '\0';
			break;
		case ELEM_OUT_COFACTOR:
			app->last_result.rows = 1;
			app->last_result.cols = 1;
			app->last_result.cell[0][0] = cofactor;
			app->has_last_label = true;
			app->last_label[0] = 'C';
			app->last_label[1] = (char)('0' + i);
			app->last_label[2] = (char)('0' + j);
			app->last_label[3] = ' ';
			app->last_label[4] = slot_name(slot);
			app->last_label[5] = '\0';
			break;
		case ELEM_OUT_BOTH:
		default:
			app->last_result.rows = 1;
			app->last_result.cols = 2;
			app->last_result.cell[0][0] = minor;
			app->last_result.cell[0][1] = cofactor;
			app->has_last_label = true;
			app->last_label[0] = 'M';
			app->last_label[1] = '/';
			app->last_label[2] = 'C';
			app->last_label[3] = ' ';
			app->last_label[4] = (char)('0' + i);
			app->last_label[5] = (char)('0' + j);
			app->last_label[6] = ' ';
			app->last_label[7] = slot_name(slot);
			app->last_label[8] = '\0';
			break;
	}

	app->result_view.cursor_row = 0;
	app->result_view.cursor_col = 0;
	nav_replace(app, SCREEN_RESULT_VIEW);
}

void screen_update_element_output(App* app, uint8_t key)
{
	UiMenu menu = {
		.title = "Output",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->element_query.out_menu_selected,
	};

	bool back = false;
	if (ui_menu_update(&menu, key, &back))
	{
		switch (app->element_query.out_menu_selected)
		{
			case 0: app->element_query.mode = ELEM_OUT_MINOR; break;
			case 1: app->element_query.mode = ELEM_OUT_COFACTOR; break;
			case 2: app->element_query.mode = ELEM_OUT_BOTH; break;
			default: app->element_query.mode = ELEM_OUT_MINOR; break;
		}
		compute_element_result(app);
	}
	else if (back)
	{
		nav_pop(app);
	}
}

void screen_draw_element_output(App* app)
{
	UiMenu menu = {
		.title = "Output",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->element_query.out_menu_selected,
	};
	ui_menu_draw(&menu);
	(void)app;
}
