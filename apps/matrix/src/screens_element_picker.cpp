#include "app.h"

#include <graphx.h>
#include <ti/getcsc.h>

static constexpr uint8_t COL_FG = 0;

void screen_enter_element_picker(App* app) { (void)app; }
void screen_exit_element_picker(App* app) { (void)app; }

void screen_update_element_picker(App* app, uint8_t key)
{
	if (key == 0)
		return;

	if (key == sk_Clear)
	{
		nav_pop(app);
		return;
	}

	const uint8_t n = app->element_query.n;
	if (n == 0)
		return;

	if (key == sk_Up)
	{
		if (app->element_query.i < n)
			++app->element_query.i;
		return;
	}
	if (key == sk_Down)
	{
		if (app->element_query.i > 1)
			--app->element_query.i;
		return;
	}
	if (key == sk_Right)
	{
		if (app->element_query.j < n)
			++app->element_query.j;
		return;
	}
	if (key == sk_Left)
	{
		if (app->element_query.j > 1)
			--app->element_query.j;
		return;
	}

	if (key == sk_Enter)
	{
		nav_push(app, SCREEN_ELEMENT_OUTPUT);
		return;
	}
}

void screen_draw_element_picker(App* app)
{
	app_draw_header("Element (i,j)");

	const uint8_t slot = app->element_query.slot;
	const uint8_t n = app->element_query.n;

	gfx_SetTextFGColor(COL_FG);
	gfx_PrintStringXY("Matrix:", 10, 36);
	gfx_PrintChar(slot_name(slot));
	gfx_PrintStringXY("Size:", 10, 52);
	gfx_SetTextXY(60, 52);
	gfx_PrintUInt(n, 1);
	gfx_PrintChar('x');
	gfx_PrintUInt(n, 1);

	gfx_PrintStringXY("i:", 10, 80);
	gfx_PrintStringXY("j:", 10, 104);

	gfx_SetColor(COL_FG);
	gfx_Rectangle(40, 76, 40, 16);
	gfx_Rectangle(40, 100, 40, 16);

	gfx_SetTextXY(55, 80);
	gfx_PrintUInt(app->element_query.i, 1);
	gfx_SetTextXY(55, 104);
	gfx_PrintUInt(app->element_query.j, 1);

	app_draw_footer("ARROWS:Set i,j  CLEAR:Back", "ENTER:Next");
}

