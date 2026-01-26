#include "app.h"

#include <debug.h>
#include <graphx.h>
#include <string.h>
#include <ti/getcsc.h>

static constexpr uint8_t COL_BG = 255;
static constexpr uint8_t COL_FG = 0;
static constexpr uint8_t COL_GRID = 0;
static constexpr uint8_t COL_CURSOR = 0;

void screen_enter_matrix_editor(App* app)
{
	app->matrix_editor.cursor_row = 0;
	app->matrix_editor.cursor_col = 0;
	app->matrix_editor.editing = false;
	app->matrix_editor.edit_len = 0;
	app->matrix_editor.edit_buf[0] = '\0';
}

void screen_exit_matrix_editor(App* app)
{
	app->matrix_editor.editing = false;
	app->matrix_editor.edit_len = 0;
	app->matrix_editor.edit_buf[0] = '\0';
	(void)app;
}

static int32_t parse_int32(const char* s)
{
	int32_t sign = 1;
	if (*s == '-')
	{
		sign = -1;
		++s;
	}
	int32_t out = 0;
	for (; *s; ++s)
	{
		if (*s < '0' || *s > '9')
			break;
		out = (int32_t)(out * 10 + (*s - '0'));
	}
	return (int32_t)(out * sign);
}

static void begin_edit_cell(App* app)
{
	const uint8_t slot = app->matrix_editor.slot;
	const MatrixSlot* m = &app->matrices[slot];
	const Rational v = m->cell[app->matrix_editor.cursor_row][app->matrix_editor.cursor_col];

	app->matrix_editor.editing = true;
	app->matrix_editor.edit_len = 0;
	app->matrix_editor.edit_buf[0] = '\0';

	// prefill with current integer value when den==1
	if (v.den == 1 && v.num != 0)
	{
		int32_t n = v.num;
		char tmp[16];
		uint8_t tlen = 0;
		if (n < 0)
		{
			tmp[tlen++] = '-';
			n = -n;
		}
		char digits[12];
		uint8_t dlen = 0;
		while (n > 0 && dlen < (uint8_t)sizeof(digits))
		{
			digits[dlen++] = (char)('0' + (n % 10));
			n /= 10;
		}
		for (uint8_t i = 0; i < dlen; ++i)
			tmp[tlen++] = digits[dlen - 1 - i];
		tmp[tlen] = '\0';
		strncpy(app->matrix_editor.edit_buf, tmp, sizeof(app->matrix_editor.edit_buf));
		app->matrix_editor.edit_len = tlen;
	}
}

static void commit_edit_cell(App* app)
{
	app->matrix_editor.editing = false;
	app->matrix_editor.edit_buf[app->matrix_editor.edit_len] = '\0';

	int32_t v = 0;
	if (app->matrix_editor.edit_len > 0 && strcmp(app->matrix_editor.edit_buf, "-") != 0)
		v = parse_int32(app->matrix_editor.edit_buf);

	MatrixSlot* m = &app->matrices[app->matrix_editor.slot];
	m->cell[app->matrix_editor.cursor_row][app->matrix_editor.cursor_col] = Rational{v, 1};

	app->matrix_editor.edit_len = 0;
	app->matrix_editor.edit_buf[0] = '\0';
}

static void cancel_edit_cell(App* app)
{
	app->matrix_editor.editing = false;
	app->matrix_editor.edit_len = 0;
	app->matrix_editor.edit_buf[0] = '\0';
}

static void edit_handle_key(App* app, uint8_t key)
{
	if (key == 0)
		return;

	if (key == sk_Clear)
	{
		cancel_edit_cell(app);
		return;
	}
	if (key == sk_Enter)
	{
		commit_edit_cell(app);
		return;
	}
	if (key == sk_Del)
	{
		if (app->matrix_editor.edit_len > 0)
		{
			--app->matrix_editor.edit_len;
			app->matrix_editor.edit_buf[app->matrix_editor.edit_len] = '\0';
		}
		return;
	}

	if (key == sk_Chs || key == sk_Sub)
	{
		if (app->matrix_editor.edit_len == 0)
		{
			app->matrix_editor.edit_buf[0] = '-';
			app->matrix_editor.edit_buf[1] = '\0';
			app->matrix_editor.edit_len = 1;
		}
		else if (app->matrix_editor.edit_buf[0] == '-')
		{
			memmove(app->matrix_editor.edit_buf, app->matrix_editor.edit_buf + 1,
			        (size_t)(app->matrix_editor.edit_len));
			--app->matrix_editor.edit_len;
			app->matrix_editor.edit_buf[app->matrix_editor.edit_len] = '\0';
		}
		else if (app->matrix_editor.edit_len + 1 < sizeof(app->matrix_editor.edit_buf))
		{
			memmove(app->matrix_editor.edit_buf + 1, app->matrix_editor.edit_buf,
			        (size_t)(app->matrix_editor.edit_len + 1));
			app->matrix_editor.edit_buf[0] = '-';
			++app->matrix_editor.edit_len;
		}
		return;
	}

	char digit = 0;
	switch (key)
	{
		case sk_0: digit = '0'; break;
		case sk_1: digit = '1'; break;
		case sk_2: digit = '2'; break;
		case sk_3: digit = '3'; break;
		case sk_4: digit = '4'; break;
		case sk_5: digit = '5'; break;
		case sk_6: digit = '6'; break;
		case sk_7: digit = '7'; break;
		case sk_8: digit = '8'; break;
		case sk_9: digit = '9'; break;
		default: break;
	}

	if (digit != 0)
	{
		if (app->matrix_editor.edit_len + 1 < sizeof(app->matrix_editor.edit_buf))
		{
			app->matrix_editor.edit_buf[app->matrix_editor.edit_len++] = digit;
			app->matrix_editor.edit_buf[app->matrix_editor.edit_len] = '\0';
		}
	}
}

void screen_update_matrix_editor(App* app, uint8_t key)
{
#ifndef NDEBUG
	if (key != 0)
	{
		dbg_printf("[matrix] editor key=%u editing=%u r=%u c=%u len=%u\n", (unsigned)key,
		           (unsigned)(app->matrix_editor.editing ? 1 : 0), (unsigned)app->matrix_editor.cursor_row,
		           (unsigned)app->matrix_editor.cursor_col, (unsigned)app->matrix_editor.edit_len);
	}
#endif
	if (app->matrix_editor.editing)
	{
		edit_handle_key(app, key);
		return;
	}

	if (key == sk_Clear)
	{
		nav_pop(app);
		return;
	}

	MatrixSlot* m = &app->matrices[app->matrix_editor.slot];

	if (key == sk_Left)
	{
		if (app->matrix_editor.cursor_col > 0)
			--app->matrix_editor.cursor_col;
		return;
	}
	if (key == sk_Right)
	{
		if (app->matrix_editor.cursor_col + 1 < m->cols)
			++app->matrix_editor.cursor_col;
		return;
	}
	if (key == sk_Up)
	{
		if (app->matrix_editor.cursor_row > 0)
			--app->matrix_editor.cursor_row;
		return;
	}
	if (key == sk_Down)
	{
		if (app->matrix_editor.cursor_row + 1 < m->rows)
			++app->matrix_editor.cursor_row;
		return;
	}

	if (key == sk_Del)
	{
		m->cell[app->matrix_editor.cursor_row][app->matrix_editor.cursor_col] = Rational{0, 1};
		return;
	}

	if (key == sk_Enter)
	{
		begin_edit_cell(app);
		return;
	}
}

static void draw_grid(const MatrixSlot* m, uint8_t cur_r, uint8_t cur_c)
{
	const int x0 = 10;
	const int y0 = 26;
	const int cell_w = 50;
	const int cell_h = 20;

	gfx_SetColor(COL_GRID);
	for (uint8_t r = 0; r <= m->rows; ++r)
		gfx_HorizLine(x0, y0 + (r * cell_h), (int)(m->cols * cell_w));
	for (uint8_t c = 0; c <= m->cols; ++c)
		gfx_VertLine(x0 + (c * cell_w), y0, (int)(m->rows * cell_h));

	gfx_SetColor(COL_CURSOR);
	gfx_FillRectangle(x0 + (cur_c * cell_w) + 1, y0 + (cur_r * cell_h) + 1, cell_w - 1, cell_h - 1);

	gfx_SetTextFGColor(COL_FG);
	for (uint8_t r = 0; r < m->rows; ++r)
	{
		for (uint8_t c = 0; c < m->cols; ++c)
		{
			const int tx = x0 + (c * cell_w) + 4;
			const int ty = y0 + (r * cell_h) + 6;
			gfx_SetTextXY(tx, ty);
			gfx_SetTextFGColor((r == cur_r && c == cur_c) ? COL_BG : COL_FG);
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

static void draw_edit_bar(const MatrixEditorState* st)
{
	gfx_SetColor(COL_BG);
	gfx_FillRectangle(0, 204, GFX_LCD_WIDTH, 18);
	gfx_SetColor(COL_GRID);
	gfx_HorizLine(0, 204, GFX_LCD_WIDTH);

	gfx_SetTextFGColor(COL_FG);
	if (st->editing)
	{
		gfx_PrintStringXY("Value:", 10, 210);
		gfx_PrintStringXY(st->edit_buf, 70, 210);
	}
}

void screen_draw_matrix_editor(App* app)
{
	MatrixSlot* m = &app->matrices[app->matrix_editor.slot];

	app_draw_header("Edit Matrix");
	gfx_SetTextFGColor(COL_FG);
	gfx_SetTextXY(250, 6);
	gfx_PrintChar(slot_name(app->matrix_editor.slot));
	gfx_PrintChar(' ');
	gfx_PrintUInt(m->rows, 1);
	gfx_PrintChar('x');
	gfx_PrintUInt(m->cols, 1);
	draw_grid(m, app->matrix_editor.cursor_row, app->matrix_editor.cursor_col);

	if (app->matrix_editor.editing)
		app_draw_footer("0-9:+  CHS:-  DEL:BKSP", "ENTER:OK  CLEAR:Cancel");
	else
		app_draw_footer("ARROWS:Move  ENTER:Edit", "DEL:0  CLEAR:Back");

	draw_edit_bar(&app->matrix_editor);
}
