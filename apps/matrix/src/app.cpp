#include "app.h"

#include <debug.h>
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#ifdef __TICE__
#include <ti/vars.h>
#endif

#include <fontlibc.h>
#include <ti/ui.h>

#include "tex/tex.h"
#include "steps.h"

static constexpr uint8_t COL_BG = 255;
static constexpr uint8_t COL_FG = 0;
static constexpr uint8_t COL_TEXT_TRANSPARENT = 254;

static void init_matrices(App* app)
{
	for (uint8_t s = 0; s < MATRIX_SLOTS; ++s)
	{
		matrix_clear(&app->matrices[s]);
	}
}

void app_init(App* app)
{
	init_matrices(app);

	app->current = SCREEN_MAIN_MENU;
	app->nav_depth = 0;
	app->running = true;

	app->has_last_result = false;
	matrix_clear(&app->last_result);
	app->has_last_label = false;
	app->last_label[0] = '\0';

	app->main_menu.selected = 0;
	app->matrices_menu.selected = 0;
	app->operations_menu.selected = 0;
	app->matrix_resize = MatrixResizeState{0, 2, 2};
	app->matrix_editor = MatrixEditorState{0, 0, 0, false, {0}, 0};

	app->slot_picker = SlotPickerState{PICKER_NONE, OP_NONE, 0, nullptr, false};
	app->op_ctx = OpContext{OP_NONE, 0, 0};
	app->result_view = ResultViewState{0, 0};
	app->steps_menu = StepsMenuState{0, 0, {nullptr}, {{0}}};
	app->element_query = ElementQueryState{0, 0, 1, 1, ELEM_OUT_MINOR, 0};
	app->has_toast = false;
	app->toast[0] = '\0';

	app->tex_renderer = nullptr;
	app->last_op = LastOpInfo{OP_NONE, 0, 0, 1, 1};
	app->last_steps = StepsLog{OP_NONE, 0, false, false, {}};
	app->steps_view = StepsViewState{0, false, nullptr, nullptr};

#ifndef NDEBUG
#ifdef __TICE__
	app->dbg_free_ram_bytes = 0;
	app->dbg_frame_counter = 0;
#endif
#endif
}

static bool ensure_tex_fonts(void)
{
	fontlib_font_t* font_main = fontlib_GetFontByIndex("TeXFonts", 0);
	fontlib_font_t* font_script = fontlib_GetFontByIndex("TeXScrpt", 0);
	if (!font_main || !font_script)
		return false;
	tex_draw_set_fonts(font_main, font_script);
	return true;
}

int app_run(App* app)
{
#ifndef NDEBUG
	dbg_ClearConsole();
	dbg_printf("[matrix] app_run start\n");
#endif
	os_RunIndicOn();

	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(COL_BG);
	gfx_SetTextTransparentColor(COL_TEXT_TRANSPARENT);
	gfx_SetTextBGColor(COL_TEXT_TRANSPARENT);
	gfx_SetTextFGColor(COL_FG);

	fontlib_SetTransparency(true);
	fontlib_SetForegroundColor(COL_FG);
	fontlib_SetBackgroundColor(COL_BG);

	const bool has_tex_fonts = ensure_tex_fonts();

	screen_enter(app, app->current);

	while (app->running)
	{
		const uint8_t key = input_poll_key();
#ifndef NDEBUG
		if (key != 0)
			dbg_printf("[matrix] key=%u screen=%u depth=%u\n", (unsigned)key, (unsigned)app->current,
			           (unsigned)app->nav_depth);
#endif
		if (key != 0 && app->has_toast)
		{
			app->has_toast = false;
			app->toast[0] = '\0';
		}
		screen_update(app, app->current, key);

#ifndef NDEBUG
#ifdef __TICE__.
		if ((uint16_t)(++app->dbg_frame_counter) % 30u == 0u)
		{
			void* free_ptr = nullptr;
			const size_t free_bytes = os_MemChk(&free_ptr);
			app->dbg_free_ram_bytes = (uint32_t)free_bytes;
		}
#endif
#endif

		gfx_FillScreen(COL_BG);
		screen_draw(app, app->current);
		app_draw_toast(app);
		app_draw_debug_overlay(app);

		if (!has_tex_fonts)
		{
			gfx_SetTextFGColor(COL_FG);
			gfx_PrintStringXY("Warning: Missing TeX font packs", 10, 220);
		}

		gfx_SwapDraw();
	}

#ifndef NDEBUG
	dbg_printf("[matrix] exiting loop; screen=%u depth=%u\n", (unsigned)app->current, (unsigned)app->nav_depth);
#endif
	screen_exit(app, app->current);

	steps_clear(&app->last_steps);
	if (app->tex_renderer)
	{
		tex_renderer_destroy(app->tex_renderer);
		app->tex_renderer = nullptr;
	}

	gfx_End();
	os_RunIndicOff();
#ifndef NDEBUG
	dbg_printf("[matrix] app_run end\n");
#endif
	return 0;
}

void nav_push(App* app, ScreenId next)
{
	if (app->nav_depth >= NAV_STACK_MAX)
		return;
#ifndef NDEBUG
	dbg_printf("[matrix] nav_push %u -> %u (depth %u)\n", (unsigned)app->current, (unsigned)next,
	           (unsigned)app->nav_depth);
#endif
	screen_exit(app, app->current);
	app->nav_stack[app->nav_depth++] = app->current;
	app->current = next;
	screen_enter(app, app->current);
}

void nav_pop(App* app)
{
#ifndef NDEBUG
	dbg_printf("[matrix] nav_pop from %u (depth %u)\n", (unsigned)app->current, (unsigned)app->nav_depth);
#endif
	if (app->nav_depth == 0)
	{
#ifndef NDEBUG
		dbg_printf("[matrix] nav_pop: stopping app\n");
#endif
		app->running = false;
		return;
	}
	screen_exit(app, app->current);
	app->current = app->nav_stack[--app->nav_depth];
#ifndef NDEBUG
	dbg_printf("[matrix] nav_pop -> %u (depth %u)\n", (unsigned)app->current, (unsigned)app->nav_depth);
#endif
	screen_enter(app, app->current);
}

void nav_replace(App* app, ScreenId next)
{
#ifndef NDEBUG
	dbg_printf("[matrix] nav_replace %u -> %u (depth %u)\n", (unsigned)app->current, (unsigned)next,
	           (unsigned)app->nav_depth);
#endif
	screen_exit(app, app->current);
	app->current = next;
	screen_enter(app, app->current);
}

char slot_name(uint8_t slot) { return (char)('A' + slot); }
