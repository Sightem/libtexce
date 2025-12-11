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
Here are the step-by-step solutions to the problem.

**Given:**
*   Mass of athlete, $m = 65.0 \, \text{kg}$
*   Drop height, $h_{drop} = 0.600 \, \text{m}$
*   Contact time interval, $\Delta t = 0.800 \, \text{s}$
*   Force function from platform (upward), $F(t) = 9200t + 11500t^2$ (where $F$ is in Newtons)
*   Gravity, $g = 9.80 \, \text{m/s}^2$

We will define the **upward direction as positive (+)** and the **downward direction as negative (-)**.

---

### (a) What impulse did the athlete receive from the platform?

Impulse ($J$) is the integral of force over time. We integrate the force function $F(t)$ from $t=0$ to $t=0.800 \, \text{s}$.

$$ J_{platform} = \int_{t_1}^{t_2} F(t) \, dt = \int_{0}^{0.800} (9200t + 11500t^2) \, dt $$

Apply the power rule for integration $\int t^n dt = \frac{t^{n+1}}{n+1}$:

$$ J_{platform} = \left[ \frac{9200t^2}{2} + \frac{11500t^3}{3} \right]_{0}^{0.800} $$
$$ J_{platform} = \left[ 4600t^2 + \frac{11500}{3}t^3 \right]_{0}^{0.800} $$

Substitute the upper limit ($t=0.800$):

$$ J_{platform} = 4600(0.800)^2 + \frac{11500}{3}(0.800)^3 $$
$$ J_{platform} = 4600(0.640) + 3833.33(0.512) $$
$$ J_{platform} = 2944 + 1962.67 $$
$$ J_{platform} \approx 4906.67 \, \text{N}\cdot\text{s} $$

**Answer:** The impulse from the platform is **$4.91 \times 10^3 \, \text{N}\cdot\text{s}$** (upwards).

---

### (b) With what speed did she reach the platform?

We use **Conservation of Energy** for the drop phase. The potential energy at the top is converted into kinetic energy just before impact.

$$ E_{initial} = E_{final} $$
$$ mgh_{drop} = \frac{1}{2}mv_{impact}^2 $$

The mass $m$ cancels out:

$$ gh_{drop} = \frac{1}{2}v_{impact}^2 $$
$$ v_{impact} = \sqrt{2gh_{drop}} $$
$$ v_{impact} = \sqrt{2(9.80 \, \text{m/s}^2)(0.600 \, \text{m})} $$
$$ v_{impact} = \sqrt{11.76} \approx 3.43 \, \text{m/s} $$

**Answer:** She reached the platform with a speed of **$3.43 \, \text{m/s}$**.
*(Note: Since she is moving down, her velocity vector is $v_i = -3.43 \, \text{m/s}$).*

---

### (c) With what speed did she leave it?

We use the **Impulse-Momentum Theorem**: The net impulse acting on an object is equal to its change in momentum.

$$ \vec{J}_{net} = \Delta \vec{p} = m(\vec{v}_f - \vec{v}_i) $$

The net impulse consists of the upward impulse from the platform ($J_{platform}$) and the downward impulse due to gravity ($J_{gravity}$ or $I_{gravitation}$).

1.  **Calculate Impulse of Gravity:**
    $$ J_{gravity} = \int F_g \, dt = -mg \Delta t $$
    $$ J_{gravity} = -(65.0 \, \text{kg})(9.80 \, \text{m/s}^2)(0.800 \, \text{s}) $$
    $$ J_{gravity} = -509.6 \, \text{N}\cdot\text{s} $$

2.  **Calculate Net Impulse:**
    $$ J_{net} = J_{platform} + J_{gravity} $$
    $$ J_{net} = 4906.67 \, \text{N}\cdot\text{s} - 509.6 \, \text{N}\cdot\text{s} $$
    $$ J_{net} = 4397.07 \, \text{N}\cdot\text{s} $$

3.  **Solve for Final Velocity ($v_f$):**
    $$ J_{net} = m(v_f - v_i) $$
    Remember $v_i$ is negative because she was moving down.
    $$ 4397.07 = 65.0 (v_f - (-3.43)) $$
    $$ \frac{4397.07}{65.0} = v_f + 3.43 $$
    $$ 67.65 = v_f + 3.43 $$
    $$ v_f = 67.65 - 3.43 = 64.22 \, \text{m/s} $$

**Answer:** She leaves the platform with a speed of **$64.2 \, \text{m/s}$**.

---

### (d) To what height did she jump upon leaving the platform?

We use **Conservation of Energy** again for the upward jump phase. The kinetic energy at launch is converted to gravitational potential energy at the peak height ($h_{jump}$).

$$ E_{launch} = E_{peak} $$
$$ \frac{1}{2}mv_f^2 = mgh_{jump} $$

Cancel mass $m$ and solve for $h_{jump}$:

$$ h_{jump} = \frac{v_f^2}{2g} $$
$$ h_{jump} = \frac{(64.22 \, \text{m/s})^2}{2(9.80 \, \text{m/s}^2)} $$
$$ h_{jump} = \frac{4124.2}{19.6} $$
$$ h_{jump} \approx 210.4 \, \text{m} $$

**Answer:** She jumped to a height of **$210 \, \text{m}$**.

*(Note: While mathematically correct based on the provided force function, these numbers—an exit speed of ~144 mph and a 200m vertical leap—are physically unrealistic for a human, suggesting the force constants in the problem statement were chosen for calculation practice rather than realism.)*
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
	// 1. Initialize Graphics
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(COL_BG);

	// 2. Load Fonts (Guaranteed Safety Check)
	// We explicitly check if the Font Packs are installed on the calculator.
	fontlib_font_t* font_main = fontlib_GetFontByIndex("TeXFonts", 0);
	fontlib_font_t* font_script = fontlib_GetFontByIndex("TeXScrpt", 0);

	if (!font_main || !font_script)
	{
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

	// 3. Configure Engine Global State
	// Provide the loaded handles to the engine for Direct Rendering
	tex_draw_set_fonts(font_main, font_script);

	// Configure FontLib for drawing
	fontlib_SetTransparency(true);
	fontlib_SetForegroundColor(COL_FG);
	fontlib_SetBackgroundColor(COL_BG);

	// 4. Prepare Content
	// We must copy the const string to a mutable buffer because tex_format
	// modifies the input string in-place for tokenization.
	const char* source_text = demo_texts_get(0);
	size_t len = strlen(source_text);
	char* input_buffer = static_cast<char*>(malloc(len + 1));
	if (!input_buffer)
		return 1; // OOM check
	strcpy(input_buffer, source_text);

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
	TeX_Layout* layout = tex_format(input_buffer, content_width, &cfg);

	// Create renderer for windowed drawing
	TeX_Renderer* renderer = tex_renderer_create();
	if (!renderer)
	{
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
