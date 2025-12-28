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

#include <stddef.h>

static constexpr const char* kText[] = {
	R"(
Calculus Reference

1. Derivative Definition
$$ f'(x) = \lim_{h \to 0} \frac{f(x+h) - f(x)}{h} $$

2. Fundamental Theorem
$$ \int_{a}^{b} f(x) \, dx = F(b) - F(a) $$

3. Taylor Series (at x=a)
$$ f(x) \approx f(a) + f'(a)(x-a) + \frac{f''(a)}{2}(x-a)^2 $$

4. Maclaurin Series
$$ e^x = \sum_{n=0}^{\infty} \frac{x^n}{n!} = 1 + x + \frac{x^2}{2} + \dots $$
$$ \sin x = \sum_{n=0}^{\infty} \frac{(-1)^n x^{2n+1}}{(2n+1)!} $$

5. Quadratic Formula
$$ x = \frac{-b \pm \sqrt{b^2 - 4ac}}{2a} $$

6. Normal Distribution
$$ P(x) = \frac{1}{\sigma \sqrt{2\pi}} e^{ -\frac{1}{2} \left( \frac{x-\mu}{\sigma} \right)^2 } $$

7. Piecewise Function
$$ f(x) = \left\{ \frac{x^2 + 1}{x - 1} \right\} $$

8. Matrices
Identity matrix:
$$ I = \begin{pmatrix}1 & 0 & 0 \\ 0 & 1 & 0 \\ 0 & 0 & 1\end{pmatrix} $$

Rotation matrix:
$$ R = \begin{bmatrix}\cos\theta & -\sin\theta & 0 \\ \sin\theta & \cos\theta & 0 \\ 0 & 0 & 1\end{bmatrix} $$

System of equations:
$$ \begin{Bmatrix}x + y = 5 \\ 2x - y = 1 \\ x + 2y = 7\end{Bmatrix} $$

Determinant:
$$ \begin{vmatrix}a & b & c \\ d & e & f \\ g & h & i\end{vmatrix} $$
)",
};

int demo_texts_count(void) { return static_cast<int>(sizeof(kText) / sizeof(kText[0])); }
const char* demo_texts_get(int idx)
{
	if (idx < 0 || idx >= demo_texts_count())
		return nullptr;
	return kText[idx];
}

// Colors
#define COL_BG 255 // White
#define COL_FG 0 // Black

int main(void)
{
	dbg_printf("start up successful\n");

	// 1. Initialize Graphics
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(COL_BG);

	dbg_printf("graphics initialized\n");

	// 2. Load Fonts (Guaranteed Safety Check)
	// We explicitly check if the Font Packs are installed on the calculator.
	fontlib_font_t* font_main = fontlib_GetFontByIndex("TeXFonts", 0);
	fontlib_font_t* font_script = fontlib_GetFontByIndex("TeXScrpt", 0);

	dbg_printf("font routine executed\n");

	if (!font_main || !font_script)
	{
		dbg_printf("fonts not loaded\n");
		// Graceful failure if AppVars are missing
		gfx_SetColor(COL_FG);
		gfx_SetTextFGColor(COL_FG);
		gfx_SetTextXY(10, 10);
		gfx_PrintString("Error: Missing Font Packs!");
		gfx_SetTextXY(10, 25);
		if (!font_main)
			gfx_PrintString("- TeXFonts.8xv missing");
		gfx_SetTextXY(10, 35);
		if (!font_script)
			gfx_PrintString("- TeXScrpt.8xv missing");

		gfx_SetTextXY(10, 60);
		gfx_PrintString("Press any key to exit.");
		gfx_SwapDraw();
		while (!os_GetCSC())
			;
		gfx_End();
		return 1;
	}

	dbg_printf("fonts loaded\n");

	// 3. Configure Engine Global State
	// Provide the loaded handles to the engine for Direct Rendering
	tex_draw_set_fonts(font_main, font_script);

	dbg_printf("engine configured\n");

	// Configure FontLib for drawing
	fontlib_SetTransparency(true);
	fontlib_SetForegroundColor(COL_FG);
	fontlib_SetBackgroundColor(COL_BG);

	dbg_printf("fontlib configured\n");

	// 4. Prepare Content
	// We must copy the const string to a mutable buffer because tex_format
	// modifies the input string in-place for tokenization.
	const char* source_text = demo_texts_get(0);
	size_t len = strlen(source_text);
	char* input_buffer = static_cast<char*>(malloc(len + 1));
	dbg_printf("input buffer allocated\n");
	if (!input_buffer)
	{
		dbg_printf("input buffer allocation failed\n");
		return 1; // OOM check
	}
	strcpy(input_buffer, source_text);
	dbg_printf("input buffer copied\n");

	// 5. Format Layout
	TeX_Config cfg = {
		.color_fg = COL_FG,
		.color_bg = COL_BG,
		.font_pack = "TeXFonts" // Logic handled manually above, but config keeps struct happy
	};

	// Margins
	const int margin = 10;
	const int content_width = GFX_LCD_WIDTH - (margin * 2);

	// Format
	dbg_printf("formatting layout\n");
	TeX_Layout* layout = tex_format(input_buffer, content_width, &cfg);
	dbg_printf("layout formatted\n");

	// Create renderer for windowed drawing
	TeX_Renderer* renderer = tex_renderer_create();
	if (!renderer)
	{
		dbg_printf("renderer creation failed\n");
		tex_free(layout);
		free(input_buffer);
		gfx_End();
		return 1;
	}

	// Calculate scroll bounds
	int total_height = tex_get_total_height(layout);
	int viewport_height = GFX_LCD_HEIGHT;
	int max_scroll = (total_height > viewport_height) ? (total_height - viewport_height) : 0;
	int scroll_y = 0;

	// 6. Main Loop
	while (true)
	{
		dbg_printf("main loop iteration\n");
		// Input
		kb_Scan();

		// Exit keys
		if (kb_Data[6] & kb_Clear)
			break;
		if (kb_Data[6] & kb_Enter)
			break;

		// Scrolling
		if (kb_Data[7] & kb_Up)
			scroll_y -= 10;
		if (kb_Data[7] & kb_Down)
			scroll_y += 10;

		// Clamp scroll
		if (scroll_y < 0)
			scroll_y = 0;
		if (scroll_y > max_scroll)
			scroll_y = max_scroll;

		// Draw Frame
		gfx_FillScreen(COL_BG);

		if (layout)
		{
			tex_draw(renderer, layout, margin, 0, scroll_y);
		}
		else
		{
			gfx_PrintStringXY("Layout Failed", 10, 10);
		}

		gfx_SwapDraw();
	}

	// 7. Cleanup - print stats before destroying
	size_t peak_used, capacity, alloc_count, reset_count;
	tex_renderer_get_stats(renderer, &peak_used, &capacity, &alloc_count, &reset_count);
	dbg_printf("[tex] Pool used: %u / %u\n", (unsigned)peak_used, (unsigned)capacity);
	dbg_printf("[tex] Total allocs: %u, resets: %u\n", (unsigned)alloc_count, (unsigned)reset_count);

	tex_renderer_destroy(renderer);
	if (layout)
		tex_free(layout);
	free(input_buffer); // Free buffer AFTER layout is freed
	gfx_End();

	return 0;
}
