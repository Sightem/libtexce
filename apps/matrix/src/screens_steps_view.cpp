#include "app.h"

#include <graphx.h>
#include <stdlib.h>
#include <string.h>
#include <ti/getcsc.h>

#include "tex/tex.h"

static constexpr uint8_t COL_BG = 255;
static constexpr uint8_t COL_FG = 0;

static void steps_free_layout(App* app)
{
	if (app->steps_view.layout)
	{
		tex_free(app->steps_view.layout);
		app->steps_view.layout = nullptr;
	}
	free(app->steps_view.layout_buf);
	app->steps_view.layout_buf = nullptr;
}

static void steps_load_layout(App* app)
{
	steps_free_layout(app);

	if (!app->last_steps.has_steps || app->last_steps.count == 0)
		return;

	const uint8_t idx = app->steps_view.index;
	if (idx >= app->last_steps.count)
		return;

	const char* latex = app->last_steps.items[idx].latex;
	if (!latex || latex[0] == '\0')
		return;

	const bool already_delimited = (strchr(latex, '$') != nullptr);
	const size_t n = strlen(latex);
	const size_t extra = already_delimited ? 0 : 4; // "$$" + "$$"
	app->steps_view.layout_buf = (char*)malloc(n + extra + 1);
	if (!app->steps_view.layout_buf)
		return;
	if (already_delimited)
	{
		memcpy(app->steps_view.layout_buf, latex, n + 1);
	}
	else
	{
		app->steps_view.layout_buf[0] = '$';
		app->steps_view.layout_buf[1] = '$';
		memcpy(app->steps_view.layout_buf + 2, latex, n);
		app->steps_view.layout_buf[2 + n] = '$';
		app->steps_view.layout_buf[2 + n + 1] = '$';
		app->steps_view.layout_buf[2 + n + 2] = '\0';
	}

	TeX_Config cfg = {
		.color_fg = COL_FG,
		.color_bg = COL_BG,
		.font_pack = "TeXFonts",
		.error_callback = nullptr,
		.error_userdata = nullptr,
	};

	const int margin_x = 10;
	const int content_width = GFX_LCD_WIDTH - (margin_x * 2);
	app->steps_view.layout = tex_format(app->steps_view.layout_buf, content_width, &cfg);
}

void screen_enter_steps_view(App* app)
{
	app->steps_view.index = 0;
	app->steps_view.pending_2nd = false;

	if (!app->tex_renderer)
		app->tex_renderer = tex_renderer_create_sized(10u * 1024u);

	steps_load_layout(app);
}

void screen_exit_steps_view(App* app)
{
	steps_free_layout(app);

	if (app->tex_renderer)
	{
		tex_renderer_destroy(app->tex_renderer);
		app->tex_renderer = nullptr;
	}
}

void screen_update_steps_view(App* app, uint8_t key)
{
	if (key == 0)
		return;

	if (key == sk_Clear)
	{
		nav_pop(app);
		return;
	}

	if (!app->last_steps.has_steps || app->last_steps.count == 0)
		return;

	if (key == sk_2nd)
	{
		app->steps_view.pending_2nd = true;
		return;
	}

	const uint8_t max_index = (uint8_t)(app->last_steps.count - 1);
	const bool use_2nd = app->steps_view.pending_2nd;
	app->steps_view.pending_2nd = false;

	if (key == sk_Left)
	{
		if (use_2nd)
		{
			if (app->steps_view.index != 0)
			{
				app->steps_view.index = 0;
				steps_load_layout(app);
			}
			return;
		}
		if (app->steps_view.index > 0)
		{
			--app->steps_view.index;
			steps_load_layout(app);
		}
		return;
	}

	if (key == sk_Right)
	{
		if (use_2nd)
		{
			if (app->steps_view.index != max_index)
			{
				app->steps_view.index = max_index;
				steps_load_layout(app);
			}
			return;
		}
		if (app->steps_view.index < max_index)
		{
			++app->steps_view.index;
			steps_load_layout(app);
		}
		return;
	}
}

void screen_draw_steps_view(App* app)
{
	app_draw_header("Steps");

	gfx_SetTextFGColor(COL_FG);
	if (!app->last_steps.has_steps || app->last_steps.count == 0)
	{
		gfx_PrintStringXY("No steps available.", 10, APP_CONTENT_TOP_Y + 20);
		app_draw_footer("CLEAR:Back", nullptr);
		return;
	}

	const uint8_t idx = app->steps_view.index;
	const uint8_t total = app->last_steps.count;

	// Step counter on the right: "k/N"
	gfx_SetTextXY(240, APP_HEADER_TEXT_Y);
	gfx_PrintUInt((unsigned)(idx + 1), 1);
	gfx_PrintChar('/');
	gfx_PrintUInt((unsigned)total, 1);

	const char* caption = app->last_steps.items[idx].caption;
	if (caption && caption[0] != '\0')
		gfx_PrintStringXY(caption, 10, APP_CONTENT_TOP_Y);

	if (app->steps_view.layout && app->tex_renderer)
	{
		const int margin_x = 10;
		const int y = APP_CONTENT_TOP_Y + 18;
		tex_draw(app->tex_renderer, app->steps_view.layout, margin_x, y, 0);
	}
	else
	{
		gfx_PrintStringXY("(render failed)", 10, APP_CONTENT_TOP_Y + 36);
	}

	if (app->last_steps.truncated)
		gfx_PrintStringXY("Note: steps truncated", 10, APP_FOOTER_LINE_Y - 14);

	app_draw_footer("CLEAR:Back  2ND+<:First  2ND+>:Last", "< >:Step");
}
