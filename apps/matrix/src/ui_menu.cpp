#include "app.h"

#include <graphx.h>
#include <ti/getcsc.h>

static constexpr uint8_t COL_FG = 0;
static constexpr uint8_t COL_HILITE_BG = 0;
static constexpr uint8_t COL_HILITE_FG = 255;

bool ui_menu_update(UiMenu* menu, uint8_t key, bool* out_back)
{
	if (out_back)
		*out_back = false;

	if (key == 0)
		return false;

	if (key == sk_Up)
	{
		if (*menu->selected == 0)
			*menu->selected = (uint8_t)(menu->count - 1);
		else
			--(*menu->selected);
		return false;
	}

	if (key == sk_Down)
	{
		*menu->selected = (uint8_t)((*menu->selected + 1) % menu->count);
		return false;
	}

	if (key == sk_Clear)
	{
		if (out_back)
			*out_back = true;
		return false;
	}

	return (key == sk_Enter);
}

void ui_menu_draw(const UiMenu* menu)
{
	app_draw_header(menu->title);

	const int x0 = 16;
	const int y0 = 30;
	const int line_h = 16;

	for (uint8_t i = 0; i < menu->count; ++i)
	{
		const int y = y0 + (i * line_h);
		if (i == *menu->selected)
		{
			gfx_SetColor(COL_HILITE_BG);
			gfx_FillRectangle(x0 - 6, y - 2, 280, line_h);
			gfx_SetTextFGColor(COL_HILITE_FG);
		}
		else
		{
			gfx_SetTextFGColor(COL_FG);
		}
		gfx_PrintStringXY(menu->items[i], x0, y);
	}

	app_draw_footer("CLEAR:Back", "ENTER:Select");
}
