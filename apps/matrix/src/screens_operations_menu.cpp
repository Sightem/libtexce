#include "app.h"

static const char* const kItems[] = {
	"Add (A+B)",
	"Subtract (A-B)",
	"Multiply (A*B)",
	"Determinant",
	"Minor/Cofactor (M_ij/C_ij)",
	"REF",
	"RREF",
	"Cramer (Ax=b)",
};

void screen_enter_operations_menu(App* app) { (void)app; }
void screen_exit_operations_menu(App* app) { (void)app; }

void screen_update_operations_menu(App* app, uint8_t key)
{
	UiMenu menu = {
		.title = "Operations",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->operations_menu.selected,
	};

	bool back = false;
	if (ui_menu_update(&menu, key, &back))
	{
		switch (app->operations_menu.selected)
		{
			case 0: app->op_ctx.op = OP_ADD; break;
			case 1: app->op_ctx.op = OP_SUB; break;
			case 2: app->op_ctx.op = OP_MUL; break;
			case 3: app->op_ctx.op = OP_DET; break;
			case 4: app->op_ctx.op = OP_ELEMENT; break;
			case 5: app->op_ctx.op = OP_REF; break;
			case 6: app->op_ctx.op = OP_RREF; break;
			case 7: app->op_ctx.op = OP_CRAMER; break;
			default: app->op_ctx.op = OP_NONE; break;
		}

		if (app->op_ctx.op == OP_NONE)
		{
			nav_push(app, SCREEN_STUB);
			return;
		}

			app->slot_picker.purpose = PICKER_OP_LHS;
			app->slot_picker.op = app->op_ctx.op;
			if (app->op_ctx.op == OP_CRAMER)
				app->slot_picker.title = "Select A";
			else if (app->op_ctx.op == OP_REF || app->op_ctx.op == OP_RREF || app->op_ctx.op == OP_DET ||
			         app->op_ctx.op == OP_ELEMENT)
				app->slot_picker.title = "Select Matrix";
			else
				app->slot_picker.title = "Select Left";
		app->slot_picker.require_set = true;
		app->slot_picker.selected = 0;
		nav_push(app, SCREEN_SLOT_PICKER);
	}
	else if (back)
	{
		nav_pop(app);
	}
}

void screen_draw_operations_menu(App* app)
{
	UiMenu menu = {
		.title = "Operations",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->operations_menu.selected,
	};
	ui_menu_draw(&menu);
	(void)app;
}
