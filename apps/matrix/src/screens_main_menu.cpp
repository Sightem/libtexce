#include "app.h"

static const char* const kItems[] = {
	"Matrices",
	"Operations",
	"Results (todo)",
	"Settings (todo)",
};

void screen_enter_main_menu(App* app) { (void)app; }
void screen_exit_main_menu(App* app) { (void)app; }

void screen_update_main_menu(App* app, uint8_t key)
{
	UiMenu menu = {
		.title = "MATRIX",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->main_menu.selected,
	};

	bool back = false;
	if (ui_menu_update(&menu, key, &back))
	{
		switch (app->main_menu.selected)
		{
			case 0:
				nav_push(app, SCREEN_MATRICES_MENU);
				break;
			case 1:
				nav_push(app, SCREEN_OPERATIONS_MENU);
				break;
			default:
				nav_push(app, SCREEN_STUB);
				break;
		}
	}
	else if (back)
	{
		nav_pop(app);
	}
}

void screen_draw_main_menu(App* app)
{
	UiMenu menu = {
		.title = "Main Menu",
		.items = kItems,
		.count = (uint8_t)(sizeof(kItems) / sizeof(kItems[0])),
		.selected = &app->main_menu.selected,
	};
	ui_menu_draw(&menu);
	(void)app;
}
