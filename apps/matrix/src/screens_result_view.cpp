#include "app.h"

#include <graphx.h>
#include <ti/getcsc.h>

static constexpr uint8_t COL_BG = 255;
static constexpr uint8_t COL_FG = 0;

static void draw_result_grid(const MatrixSlot* m, uint8_t cur_r, uint8_t cur_c)
{
	const int x0 = 10;
	const int y0 = 26;
	const int cell_w = 50;
	const int cell_h = 20;

	gfx_SetColor(COL_FG);
	for (uint8_t r = 0; r <= m->rows; ++r)
		gfx_HorizLine(x0, y0 + (r * cell_h), (int)(m->cols * cell_w));
	for (uint8_t c = 0; c <= m->cols; ++c)
		gfx_VertLine(x0 + (c * cell_w), y0, (int)(m->rows * cell_h));

	gfx_SetColor(COL_FG);
	gfx_FillRectangle(x0 + (cur_c * cell_w) + 1, y0 + (cur_r * cell_h) + 1, cell_w - 1, cell_h - 1);

	for (uint8_t r = 0; r < m->rows; ++r)
	{
		for (uint8_t c = 0; c < m->cols; ++c)
		{
			const int tx = x0 + (c * cell_w) + 4;
			const int ty = y0 + (r * cell_h) + 6;
			gfx_SetTextXY(tx, ty);

			const bool is_sel = (r == cur_r && c == cur_c);
			gfx_SetTextFGColor(is_sel ? COL_BG : COL_FG);

			const Rational v = m->cell[r][c];
			if (v.den == 1)
			{
				gfx_PrintInt((int)v.num, 1);
			}
			else
			{
				gfx_PrintInt((int)v.num, 1);
				gfx_PrintChar('/');
				gfx_PrintInt((int)v.den, 1);
			}
		}
	}
}

static void draw_result_label(const App* app)
{
	gfx_SetTextFGColor(COL_FG);
	gfx_SetTextXY(235, 6);
	if (app->has_last_label && app->last_label[0] != '\0')
		gfx_PrintString(app->last_label);
}

void screen_enter_result_view(App* app) { (void)app; }
void screen_exit_result_view(App* app) { (void)app; }

void screen_update_result_view(App* app, uint8_t key)
{
	if (key == 0)
		return;

	if (key == sk_Clear)
	{
		nav_pop(app);
		return;
	}

	if (!app->has_last_result)
		return;

	if (key == sk_Enter)
	{
		app->slot_picker.purpose = PICKER_SAVE_RESULT;
		app->slot_picker.title = "Save Result";
		app->slot_picker.require_set = false;
		app->slot_picker.selected = 0;
		nav_push(app, SCREEN_SLOT_PICKER);
		return;
	}

	if (key == sk_2nd)
	{
		if (!app->last_steps.has_steps || app->last_steps.count == 0)
		{
			app->has_toast = true;
			app->toast[0] = 'N';
			app->toast[1] = 'o';
			app->toast[2] = ' ';
			app->toast[3] = 's';
			app->toast[4] = 't';
			app->toast[5] = 'e';
			app->toast[6] = 'p';
			app->toast[7] = 's';
			app->toast[8] = '\0';
			return;
		}
		if (app->last_op.op == OP_CRAMER)
			nav_push(app, SCREEN_STEPS_MENU);
		else
			nav_push(app, SCREEN_STEPS_VIEW);
		return;
	}

	const MatrixSlot* m = &app->last_result;
	if (key == sk_Left)
	{
		if (app->result_view.cursor_col > 0)
			--app->result_view.cursor_col;
	}
	else if (key == sk_Right)
	{
		if (app->result_view.cursor_col + 1 < m->cols)
			++app->result_view.cursor_col;
	}
	else if (key == sk_Up)
	{
		if (app->result_view.cursor_row > 0)
			--app->result_view.cursor_row;
	}
	else if (key == sk_Down)
	{
		if (app->result_view.cursor_row + 1 < m->rows)
			++app->result_view.cursor_row;
	}
}

void screen_draw_result_view(App* app)
{
	app_draw_header("Result");
	if (!app->has_last_result)
	{
		gfx_SetTextFGColor(COL_FG);
		gfx_PrintStringXY("No result yet.", 10, 60);
		app_draw_footer("CLEAR:Back", nullptr);
		return;
	}

	draw_result_label(app);
	draw_result_grid(&app->last_result, app->result_view.cursor_row, app->result_view.cursor_col);
	app_draw_footer("CLEAR:Back  2ND:Steps", "ENTER:Save");
}
