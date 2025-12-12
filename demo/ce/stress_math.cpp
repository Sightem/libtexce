#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <debug.h>
#include <fontlibc.h>
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include "tex/tex.h"
#include "tex/tex_internal.h"

// Stress test: 500+ repeated math symbols to establish baseline memory usage.
// Run this on CE emulator and check debug output for pool stats.

// Generate a stress test string: "x + y + z + x + y + z + ..."
// Each repetition is ~10 chars: "x + y + z "
// 50 repetitions = 500 chars
// NOTE: Removed raw string literal to avoid leading newline tokenization issues
static const char* kStressMath =
    "$$ x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x "
    "+ y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + "
    "z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x "
    "+ y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + z + x + y + "
    "z + x + y + z + x + y + z + x + y + z + x + y + z + x + y^2 + z_1 $$";

// Colors
#define COL_BG 255 // White
#define COL_FG 0 // Black

int main(void)
{
	dbg_printf("[stress_math] Starting stress test...\n");

	// 1. Initialize Graphics
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(COL_BG);

	// 2. Load Fonts
	fontlib_font_t* font_main = fontlib_GetFontByIndex("TeXFonts", 0);
	fontlib_font_t* font_script = fontlib_GetFontByIndex("TeXScrpt", 0);

	if (!font_main || !font_script)
	{
		gfx_SetColor(COL_FG);
		gfx_SetTextFGColor(COL_FG);
		gfx_SetTextXY(10, 10);
		gfx_PrintString("Error: Missing Font Packs!");
		gfx_SwapDraw();
		while (!os_GetCSC())
			;
		gfx_End();
		return 1;
	}

	// 3. Configure Engine
	tex_draw_set_fonts(font_main, font_script);
	fontlib_SetTransparency(true);
	fontlib_SetForegroundColor(COL_FG);
	fontlib_SetBackgroundColor(COL_BG);

	// 4. Copy stress string to mutable buffer
	size_t len = strlen(kStressMath);
	char* input_buffer = static_cast<char*>(malloc(len + 1));
	if (!input_buffer)
	{
		dbg_printf("[stress_math] OOM allocating input buffer\n");
		gfx_End();
		return 1;
	}
	strcpy(input_buffer, kStressMath);

	// 5. Format Layout
	TeX_Config cfg = { .color_fg = COL_FG, .color_bg = COL_BG, .font_pack = "TeXFonts" };

	const int margin = 10;
	const int content_width = GFX_LCD_WIDTH - (margin * 2);

	dbg_printf("[stress_math] Formatting layout (width=%d)...\n", content_width);
	TeX_Layout* layout = tex_format(input_buffer, content_width, &cfg);

	// Create renderer
	TeX_Renderer* renderer = tex_renderer_create();
	if (!renderer)
	{
		dbg_printf("[stress_math] Failed to create renderer\n");
		tex_free(layout);
		free(input_buffer);
		gfx_End();
		return 1;
	}

	// Calculate scroll bounds
	int total_height = tex_get_total_height(layout);
	// int viewport_height = GFX_LCD_HEIGHT;
	//  int max_scroll = (total_height > viewport_height) ? (total_height - viewport_height) : 0;
	int scroll_y = 0;

	dbg_printf("[stress_math] Total height: %d\n", total_height);

	// 6. Draw once to populate pool
	gfx_FillScreen(COL_BG);
	if (layout)
	{
		tex_draw(renderer, layout, margin, 0, scroll_y);
	}
	gfx_SwapDraw();

	// 7. Print baseline stats
	size_t peak_used, capacity, alloc_count, reset_count;
	tex_renderer_get_stats(renderer, &peak_used, &capacity, &alloc_count, &reset_count);

	dbg_printf("==== STRESS TEST BASELINE ====\n");
	dbg_printf("[stress_math] Pool peak_used: %u bytes\n", (unsigned)peak_used);
	dbg_printf("[stress_math] Pool capacity:  %u bytes\n", (unsigned)capacity);
	dbg_printf("[stress_math] Total allocs:   %u\n", (unsigned)alloc_count);
	dbg_printf("[stress_math] Pool resets:    %u\n", (unsigned)reset_count);
	dbg_printf("[stress_math] sizeof(Node):   %u bytes\n", (unsigned)sizeof(Node));
	dbg_printf("==============================\n");

	// 8. Wait for keypress then exit
	gfx_SetTextXY(10, 10);
	gfx_PrintString("Stress test complete.");
	gfx_SetTextXY(10, 25);
	gfx_PrintString("Check debug output.");
	gfx_SetTextXY(10, 45);
	gfx_PrintString("Press any key to exit.");
	gfx_SwapDraw();

	while (!os_GetCSC())
		;

	// 9. Cleanup
	tex_renderer_destroy(renderer);
	if (layout)
		tex_free(layout);
	free(input_buffer);
	gfx_End();

	return 0;
}
