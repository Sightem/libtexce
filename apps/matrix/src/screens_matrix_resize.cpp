#include "app.h"

#include <graphx.h>
#include <ti/getcsc.h>

static constexpr uint8_t COL_FG = 0;

void screen_enter_matrix_resize(App* app) { (void)app; }
void screen_exit_matrix_resize(App* app) { (void)app; }

static void apply_resize(App* app)
{
	const uint8_t slot = app->matrix_resize.slot;
	MatrixSlot* m = &app->matrices[slot];
	m->rows = app->matrix_resize.rows;
	m->cols = app->matrix_resize.cols;

	for (uint8_t r = 0; r < MATRIX_MAX_ROWS; ++r)
	{
		for (uint8_t c = 0; c < MATRIX_MAX_COLS; ++c)
		{
			if (r >= m->rows || c >= m->cols)
				m->cell[r][c] = Rational{0, 1};
		}
	}
}

void screen_update_matrix_resize(App* app, uint8_t key)
{
	if (key == sk_Clear)
	{
		nav_pop(app);
		return;
	}

	if (key == sk_Left)
	{
		if (app->matrix_resize.cols > 1)
			--app->matrix_resize.cols;
		return;
	}

	if (key == sk_Right)
	{
		if (app->matrix_resize.cols < MATRIX_MAX_COLS)
			++app->matrix_resize.cols;
		return;
	}

	if (key == sk_Up)
	{
		if (app->matrix_resize.rows < MATRIX_MAX_ROWS)
			++app->matrix_resize.rows;
		return;
	}

	if (key == sk_Down)
	{
		if (app->matrix_resize.rows > 1)
			--app->matrix_resize.rows;
		return;
	}

	if (key == sk_Enter)
	{
		apply_resize(app);
		app->matrix_editor.slot = app->matrix_resize.slot;
		app->matrix_editor.cursor_row = 0;
		app->matrix_editor.cursor_col = 0;
		app->matrix_editor.editing = false;
		app->matrix_editor.edit_len = 0;
		nav_replace(app, SCREEN_MATRIX_EDITOR);
	}
}

void screen_draw_matrix_resize(App* app)
{
	char title[16];
	title[0] = slot_name(app->matrix_resize.slot);
	title[1] = '\0';

	app_draw_header("Resize Matrix");

	gfx_SetTextFGColor(COL_FG);
	gfx_PrintStringXY("Slot:", 10, 40);
	gfx_PrintStringXY(title, 60, 40);

	gfx_PrintStringXY("Rows:", 10, 70);
	gfx_PrintStringXY("Cols:", 10, 95);

	gfx_SetColor(COL_FG);
	gfx_Rectangle(60, 66, 40, 16);
	gfx_Rectangle(60, 91, 40, 16);

	gfx_SetTextFGColor(COL_FG);
	gfx_SetTextXY(70, 70);
	gfx_PrintUInt(app->matrix_resize.rows, 1);

	gfx_SetTextXY(70, 95);
	gfx_PrintUInt(app->matrix_resize.cols, 1);

	app_draw_footer("ARROWS:Adjust  CLEAR:Back", "ENTER:OK");
}
