#include "app.h"

#include <graphx.h>

static const char* const kItems[] = {
	"Edit A",
	"Edit B",
	"Edit C",
	"Edit D",
	"Clear Slot",
};

void screen_enter_matrices_menu(App* app) { (void)app; }
void screen_exit_matrices_menu(App* app) { (void)app; }

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
		if (m->rows == 0 || m->cols == 0)
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

void screen_update_matrices_menu(App* app, uint8_t key)
{
	UiMenu menu = {
		.title = "Matrices",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->matrices_menu.selected,
	};

	bool back = false;
	if (ui_menu_update(&menu, key, &back))
	{
		if (app->matrices_menu.selected <= 3)
		{
			const uint8_t slot = app->matrices_menu.selected;
			app->matrix_editor.slot = slot;
			app->matrix_resize.slot = slot;

			if (app->matrices[slot].rows == 0 || app->matrices[slot].cols == 0)
			{
				app->matrix_resize.rows = 2;
				app->matrix_resize.cols = 2;
				nav_push(app, SCREEN_MATRIX_RESIZE);
			}
			else
			{
				nav_push(app, SCREEN_MATRIX_EDITOR);
			}
		}
		else
		{
			app->slot_picker.purpose = PICKER_CLEAR_SLOT;
			app->slot_picker.title = "Clear Slot";
			app->slot_picker.require_set = false;
			app->slot_picker.selected = 0;
			nav_push(app, SCREEN_SLOT_PICKER);
		}
	}
	else if (back)
	{
		nav_pop(app);
	}
}

void screen_draw_matrices_menu(App* app)
{
	UiMenu menu = {
		.title = "Matrices",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->matrices_menu.selected,
	};
	ui_menu_draw(&menu);
	draw_slot_sizes(app);
}
