#include "app.h"

#include <graphx.h>

static constexpr uint8_t COL_FG = 0;

void app_draw_header(const char* title)
{
	gfx_SetTextFGColor(COL_FG);
	gfx_PrintStringXY(title, 10, APP_HEADER_TEXT_Y);
	gfx_SetColor(COL_FG);
	gfx_HorizLine(0, APP_HEADER_LINE_Y, GFX_LCD_WIDTH);
}

void app_draw_footer(const char* left, const char* right)
{
	gfx_SetColor(COL_FG);
	gfx_HorizLine(0, APP_FOOTER_LINE_Y, GFX_LCD_WIDTH);
	gfx_SetTextFGColor(COL_FG);
	if (left)
		gfx_PrintStringXY(left, 10, APP_FOOTER_TEXT_Y);
	if (right)
		gfx_PrintStringXY(right, 200, APP_FOOTER_TEXT_Y);
}

void app_draw_toast(App* app)
{
	if (!app->has_toast)
		return;

	gfx_SetTextFGColor(COL_FG);
	gfx_PrintStringXY(app->toast, 10, 206);
}

void app_draw_debug_overlay(const App* app)
{
#if !defined(NDEBUG) && defined(__TICE__)
	// Keep this unobtrusive and cheap; helps verify free RAM during idle.
	gfx_SetTextFGColor(COL_FG);
	const int x = 180;
	const int y = APP_FOOTER_LINE_Y - 14;
	gfx_PrintStringXY("RAM:", x, y);
	gfx_PrintUInt((unsigned)app->dbg_free_ram_bytes, 1);
#else
	(void)app;
#endif
}
