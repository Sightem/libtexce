#include "app.h"
#include "steps.h"

#include <graphx.h>
#include <string.h>
#include <ti/getcsc.h>

static const char* const kItems[] = {
	"A",
	"B",
	"C",
	"D",
};

void screen_enter_slot_picker(App* app) { (void)app; }
void screen_exit_slot_picker(App* app) { (void)app; }

static void draw_slot_sizes(const App* app)
{
	const int x = 220;
	const int y0 = 30;
	const int line_h = 16;

	for (uint8_t i = 0; i < MATRIX_SLOTS; ++i)
	{
		const MatrixSlot* m = &app->matrices[i];
		const int y = y0 + (i * line_h);
		gfx_SetTextXY(x, y);
		if (!matrix_is_set(m))
		{
			gfx_PrintString("--");
		}
		else
		{
			gfx_PrintUInt(m->rows, 1);
			gfx_PrintChar('x');
			gfx_PrintUInt(m->cols, 1);
		}
	}
}

static void set_toast(App* app, const char* msg)
{
	app->has_toast = true;
	strncpy(app->toast, msg, sizeof(app->toast));
	app->toast[sizeof(app->toast) - 1] = '\0';
}

static void compute_and_show_result(App* app)
{
	const MatrixSlot* a = &app->matrices[app->op_ctx.lhs_slot];
	const MatrixSlot* b = &app->matrices[app->op_ctx.rhs_slot];

	steps_clear(&app->last_steps);
	(void)steps_begin(&app->last_steps, app->op_ctx.op);

	bool ok = false;
	switch (app->op_ctx.op)
	{
		case OP_ADD:
			ok = matrix_add(a, b, &app->last_result);
			if (ok)
			{
				(void)steps_append_matrix(&app->last_steps, "A", a);
				(void)steps_append_matrix(&app->last_steps, "B", b);
				(void)steps_append_matrix(&app->last_steps, "A+B", &app->last_result);
			}
			break;
		case OP_SUB:
			ok = matrix_sub(a, b, &app->last_result);
			if (ok)
			{
				(void)steps_append_matrix(&app->last_steps, "A", a);
				(void)steps_append_matrix(&app->last_steps, "B", b);
				(void)steps_append_matrix(&app->last_steps, "A-B", &app->last_result);
			}
			break;
		case OP_MUL:
			ok = matrix_mul(a, b, &app->last_result);
			if (ok)
			{
				(void)steps_append_matrix(&app->last_steps, "A", a);
				(void)steps_append_matrix(&app->last_steps, "B", b);
				(void)steps_append_matrix(&app->last_steps, "A*B", &app->last_result);
			}
			break;
		case OP_REF: ok = matrix_ref_steps(a, &app->last_result, &app->last_steps); break;
		case OP_RREF: ok = matrix_rref_steps(a, &app->last_result, &app->last_steps); break;
		case OP_DET: {
			Rational det = Rational{0, 1};
			ok = matrix_det_steps(a, &det, &app->last_steps);
			if (ok)
			{
				matrix_clear(&app->last_result);
				app->last_result.rows = 1;
				app->last_result.cols = 1;
				app->last_result.cell[0][0] = det;
				(void)steps_append_matrix(&app->last_steps, "det(A)", &app->last_result);
			}
		}
		break;
		case OP_CRAMER: {
			const CramerStatus st = matrix_cramer(a, b, &app->last_result);
			if (st == CRAMER_INVALID)
			{
				set_toast(app, "Need A nxn and B nx1");
				steps_clear(&app->last_steps);
				return;
			}
			if (st == CRAMER_SINGULAR)
			{
				set_toast(app, "No unique solution");
				steps_clear(&app->last_steps);
				return;
			}
			ok = (st == CRAMER_OK);
			if (ok)
			{
				// store lightweight overview now, deeper det steps are generated on demand
				(void)steps_append_matrix(&app->last_steps, "A", a);
				(void)steps_append_matrix(&app->last_steps, "b", b);
				(void)steps_append_tex(&app->last_steps, "Cramer's rule",
				                       "\\Delta=\\det(A),\\ \\Delta_i=\\det(A_i),\\ x_i=\\frac{\\Delta_i}{\\Delta}");
				(void)steps_append_matrix(&app->last_steps, "x", &app->last_result);
			}
		}
		break;
		default: ok = false; break;
	}

	if (!ok)
	{
		steps_clear(&app->last_steps);
		if (app->op_ctx.op == OP_MUL)
			set_toast(app, "Need L.cols = R.rows");
		else if (app->op_ctx.op == OP_ADD || app->op_ctx.op == OP_SUB)
			set_toast(app, "Need same size");
		else if (app->op_ctx.op == OP_DET)
			set_toast(app, "Need square matrix");
		else
			set_toast(app, "Failed");
		return;
	}

	app->has_last_result = true;
	app->has_last_label = true;
	app->last_op.op = app->op_ctx.op;
	app->last_op.lhs_slot = app->op_ctx.lhs_slot;
	app->last_op.rhs_slot = app->op_ctx.rhs_slot;
	app->last_op.i = 1;
	app->last_op.j = 1;
	switch (app->op_ctx.op)
	{
		case OP_ADD:
			app->last_label[0] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[1] = '+';
			app->last_label[2] = slot_name(app->op_ctx.rhs_slot);
			app->last_label[3] = '\0';
			break;
		case OP_SUB:
			app->last_label[0] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[1] = '-';
			app->last_label[2] = slot_name(app->op_ctx.rhs_slot);
			app->last_label[3] = '\0';
			break;
		case OP_MUL:
			app->last_label[0] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[1] = '*';
			app->last_label[2] = slot_name(app->op_ctx.rhs_slot);
			app->last_label[3] = '\0';
			break;
		case OP_REF:
			app->last_label[0] = 'R';
			app->last_label[1] = 'E';
			app->last_label[2] = 'F';
			app->last_label[3] = ' ';
			app->last_label[4] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[5] = '\0';
			break;
		case OP_RREF:
			app->last_label[0] = 'R';
			app->last_label[1] = 'R';
			app->last_label[2] = 'E';
			app->last_label[3] = 'F';
			app->last_label[4] = ' ';
			app->last_label[5] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[6] = '\0';
			break;
		case OP_DET:
			app->last_label[0] = 'D';
			app->last_label[1] = 'E';
			app->last_label[2] = 'T';
			app->last_label[3] = ' ';
			app->last_label[4] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[5] = '\0';
			break;
		case OP_CRAMER:
			app->last_label[0] = 'C';
			app->last_label[1] = 'R';
			app->last_label[2] = 'A';
			app->last_label[3] = 'M';
			app->last_label[4] = ' ';
			app->last_label[5] = slot_name(app->op_ctx.lhs_slot);
			app->last_label[6] = ',';
			app->last_label[7] = slot_name(app->op_ctx.rhs_slot);
			app->last_label[8] = '\0';
			break;
		default:
			app->has_last_label = false;
			app->last_label[0] = '\0';
			break;
	}
	app->result_view.cursor_row = 0;
	app->result_view.cursor_col = 0;
	nav_replace(app, SCREEN_RESULT_VIEW);
}

void screen_update_slot_picker(App* app, uint8_t key)
{
	if (key == 0)
		return;

	UiMenu menu = {
		.title = (app->slot_picker.title != nullptr) ? app->slot_picker.title : "Select Slot",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->slot_picker.selected,
	};

	bool back = false;
	const bool activated = ui_menu_update(&menu, key, &back);

	if (back)
	{
		if (app->slot_picker.purpose == PICKER_OP_RHS)
		{
			app->slot_picker.purpose = PICKER_OP_LHS;
			app->slot_picker.title = "Select Left";
			app->slot_picker.selected = app->op_ctx.lhs_slot;
			return;
		}
		nav_pop(app);
		return;
	}

	if (!activated)
		return;

	const uint8_t slot = app->slot_picker.selected;
	const bool is_set = matrix_is_set(&app->matrices[slot]);
	if (app->slot_picker.require_set && !is_set)
	{
		char msg[16];
		msg[0] = 'S';
		msg[1] = 'l';
		msg[2] = 'o';
		msg[3] = 't';
		msg[4] = ' ';
		msg[5] = slot_name(slot);
		msg[6] = ' ';
		msg[7] = 'e';
		msg[8] = 'm';
		msg[9] = 'p';
		msg[10] = 't';
		msg[11] = 'y';
		msg[12] = '\0';
		set_toast(app, msg);
		return;
	}

	switch (app->slot_picker.purpose)
	{
		case PICKER_CLEAR_SLOT:
			matrix_clear(&app->matrices[slot]);
			{
				char msg[12];
				msg[0] = 'C';
				msg[1] = 'l';
				msg[2] = 'e';
				msg[3] = 'a';
				msg[4] = 'r';
				msg[5] = 'e';
				msg[6] = 'd';
				msg[7] = ' ';
				msg[8] = slot_name(slot);
				msg[9] = '\0';
				set_toast(app, msg);
			}
			nav_pop(app);
			return;

		case PICKER_SAVE_RESULT:
			if (!app->has_last_result)
			{
				set_toast(app, "No result to save");
				nav_pop(app);
				return;
			}
			app->matrices[slot] = app->last_result;
			{
				char msg[16];
				msg[0] = 'S';
				msg[1] = 'a';
				msg[2] = 'v';
				msg[3] = 'e';
				msg[4] = 'd';
				msg[5] = ' ';
				msg[6] = '-';
				msg[7] = '>';
				msg[8] = ' ';
				msg[9] = slot_name(slot);
				msg[10] = '\0';
				set_toast(app, msg);
			}
			nav_pop(app);
			return;

		case PICKER_OP_LHS:
			app->op_ctx.lhs_slot = slot;
			if (app->op_ctx.op == OP_ELEMENT)
			{
				const MatrixSlot* m = &app->matrices[slot];
				if (m->rows != m->cols)
				{
					set_toast(app, "Need square matrix");
					return;
				}
				app->element_query.slot = slot;
				app->element_query.n = m->rows;
				app->element_query.i = 1;
				app->element_query.j = 1;
				app->element_query.mode = ELEM_OUT_MINOR;
				app->element_query.out_menu_selected = 0;
				nav_push(app, SCREEN_ELEMENT_PICKER);
				return;
			}
			if (app->op_ctx.op == OP_REF || app->op_ctx.op == OP_RREF || app->op_ctx.op == OP_DET)
			{
				compute_and_show_result(app);
			}
			else
			{
				app->slot_picker.purpose = PICKER_OP_RHS;
				app->slot_picker.title = (app->op_ctx.op == OP_CRAMER) ? "Select b" : "Select Right";
				app->slot_picker.require_set = true;
				app->slot_picker.selected = 0;
			}
			return;

		case PICKER_OP_RHS:
			app->op_ctx.rhs_slot = slot;
			compute_and_show_result(app);
			return;

		case PICKER_NONE:
		default:
			nav_pop(app);
			return;
	}
}

void screen_draw_slot_picker(App* app)
{
	UiMenu menu = {
		.title = (app->slot_picker.title != nullptr) ? app->slot_picker.title : "Select Slot",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->slot_picker.selected,
	};
	ui_menu_draw(&menu);
	draw_slot_sizes(app);
}
