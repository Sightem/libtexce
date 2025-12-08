#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fontlibc.h>
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include "tex/tex.h"

#include <stddef.h>

static constexpr const char* kText[] = {
	R"(


Test 1.1: $abc$ (expect: one text node)
Test 1.2: $abcdefghij$ (expect: one text node)
Test 1.3: $abc + def$ (expect: N_TEXT(abc), N_GLYPH(+), N_TEXT(def))
Test 1.4: $12345$ (expect: one text node)
Test 1.5: $abc123xyz$ (expect: one text node)

Test 2.1: $x^2$ (base must be single glyph)
Test 2.2: $abcx^2$ (expect: N_TEXT(abc), N_SCRIPT(x,2))
Test 2.3: $x^2abc$ (expect: N_SCRIPT(x,2), N_TEXT(abc))
Test 2.4: $x^2 + y^2$ (two separate scripts)
Test 2.5: $a^1b^2c^3$ (three scripts, no consolidation)
Test 2.6: $x_1 + y_2$ (subscripts)
Test 2.7: $x_1^2$ (both sub and super)
Test 2.8: $abc x^2 def$ (expect: N_TEXT(abc), space, script, space, N_TEXT(def))

Test 3.1: $\frac{abc}{xyz}$ (text in both num and den)
Test 3.2: $\frac{x^2 + y^2}{abc}$ (scripts in num, text in den)
Test 3.3: $\frac{1}{1+\frac{abc}{def}}$ (nested with text)
Test 3.4: $\frac{abcd + efgh}{ijkl - mnop}$ (longer text runs)

Test 4.1: $\sqrt{abc}$ (text under radical)
Test 4.2: $\sqrt{x^2 + y^2}$ (scripts under radical)
Test 4.3: $\sqrt[3]{abc}$ (cube root with text)
Test 4.4: $\sqrt[abc]{xyz}$ (text in index and radicand)

Test 5.1: ${abc}$ (braced text)
Test 5.2: $x^{abc}$ (text in superscript group)
Test 5.3: $x_{abc}$ (text in subscript group)
Test 5.4: ${{abc}{def}}$ (nested groups with text)
Test 5.5: ${a^2 b^2}$ (scripts inside group)

Test 6.1: $\alpha$ (single symbol)
Test 6.2: $\alpha abc$ (symbol then text)
Test 6.3: $abc \alpha$ (text then symbol)
Test 6.4: $\alpha\beta\gamma$ (consecutive symbols)
Test 6.5: $\alpha^2$ (symbol with superscript)
Test 6.6: $\alpha + \beta = \gamma$ (symbols with operators)

Test 7.1: $abc + def - ghi$ (operators separate runs)
Test 7.2: $abc \times def$ (times operator)
Test 7.3: $abc = def$ (equals)
Test 7.4: $(abc)(def)$ (parenthesized groups)
Test 7.5: $[abc]$ (bracketed text)

Test 8.1: $\sum_{abc}^{xyz}$ (text in sum limits)
Test 8.2: $\int_{abc}^{xyz}$ (text in integral limits)
Test 8.3: $\prod_{i}^{n} abc$ (product followed by text)

Test 9.1: $\hat{x}$ (hat accent)
Test 9.2: $\bar{abc}$ (bar over text)
Test 9.3: $\vec{v}$ (vector)
Test 9.4: $\overbrace{abcdef}^{text}$ (overbrace)
Test 9.5: $\underbrace{abcdef}_{text}$ (underbrace)

Test 10.1: $\sin abc$ (function with text arg)
Test 10.2: $\log_{abc} xyz$ (log with text base and arg)
Test 10.3: $\lim_{x \to abc} xyz$ (limit with text)

Test 11.1: $\frac{-b \pm \sqrt{b^2 - 4ac}}{2a}$ (quadratic formula)
Test 11.2: $E = mc^2$ (energy equation)
Test 11.3: $ax^3 + bx^2 + cx + d$ (polynomial)
Test 11.4: $\sum_{i=1}^{n} (abc_i + def_i)$ (summation)
Test 11.5: $\frac{abc + \frac{def}{ghi}}{jkl + mno}$ (nested fractions)
Test 11.6: $abc + def + ghi + jkl + mno + pqr$ (many text runs)

Test 12.2: $x$ (single char - becomes N_GLYPH)
Test 12.3: $xy$ (two chars - becomes N_TEXT)
Test 12.4: $a \quad b$ (explicit space)
Test 12.5: ${abc}^2$ (group with script)
Test 12.6: $x^2^3$ (should handle gracefully)
Test 12.7: $^2$ (superscript without base)
Test 12.8: $abcdefghijklmnopqrstuvwxyz$ (26 letters)

Test 13.1:
$$abcdef + ghijkl = mnopqr$$
(display math with text runs)

Test 13.2:
$$\frac{abcdefgh}{ijklmnop}$$
(display fraction with text)

Test 13.3:
$$abc + def + ghi + jkl$$
(display with operators)

Test 14.1: $\left(\frac{1}{2}\right)$ (extensible parentheses)
Test 14.2: $\left[\sqrt{x^2 + y^2}\right]$ (extensible brackets)
Test 14.3: $\left\{\sum_{i=1}^n i\right\}$ (extensible braces)
Test 14.4: $\left.\frac{x^2}{2}\right|_0^1$ (invisible left, vert right)
Test 14.5: $\left(\frac{\frac{a}{b}}{\frac{c}{d}}\right)$ (deeply nested)
Test 14.6: $\left\langle x, y \right\rangle$ (angle brackets)

Here are the step-by-step solutions. Parts (b), (c), and (d) are solved algebraically as requested, while parts (e) and (f) include the numerical calculations.

(a) Free Body Diagrams
Although I cannot draw directly on the page, here is the description of the forces for the Free Body Diagrams:
1.  For the Hanging Mass ($m$):
    Gravity ($F_g = mg$): Pointing vertically downward.
    Tension ($T$): Pointing vertically upward along the string.
    Note: Since the object accelerates downward, $mg > T$.
2.  For the Reel ($M$):
    Tension ($T$): Pointing downward, applied tangentially at the rim (radius $R$) on the right side. This creates the torque.
    Force from Axle ($N$ or $F_{axle}$): Pointing upward at the center to support the reel's weight and the downward tension.
    Gravity ($F_g = Mg$): Pointing downward at the center of the reel.

(b) Expression for Angular Acceleration ($\alpha$)
We apply Newton's Second Law for rotation to the reel and translation to the mass.
1.  Torque on the reel:
$$ \tau_{net} = I\alpha $$
The torque is caused by tension $T$ at radius $R$:
$$ TR = I\alpha \quad arrow \quad T = \frac{I\alpha}{R} $$
2.  Force on the hanging mass:
$$ F_{net} = ma $$
$$ mg - T = ma $$
3.  Relationship between linear and angular acceleration:
Since the string does not slip:
$$ a = R\alpha $$
4.  Substitute and Solve:
Substitute $T$ from the torque equation and $a$ from the geometric constraint into the force equation:
$$ mg - ( \frac{I\alpha}{R} ) = m(R\alpha) $$
Move $\alpha$ terms to one side:
$$ mg = mR\alpha + \frac{I\alpha}{R} $$
$$ mg = \alpha ( mR + \frac{I}{R} ) $$
Solve for $\alpha$:
$$ \alpha = \frac{mg}{mR + \frac{I}{R}} $$
Substitute the moment of inertia for a solid disk $I = \frac{1}{2}MR^2$:
$$ \alpha = \frac{mg}{mR + \frac{\frac{1}{2}MR^2}{R}} = \frac{mg}{mR + \frac{1}{2}MR} $$
$$ \alpha = \frac{mg}{R(m + \frac{1}{2}M)} $$
Answer:
$$ \alpha = \frac{mg}{R(m + \frac{M}{2})} $$

(c) Expression for Translational Acceleration ($a$)
We use the relationship $a = R\alpha$.
$$ a = R [ \frac{mg}{R(m + \frac{M}{2})} ] $$
The $R$ cancels out:
Answer:
$$ a = \frac{mg}{m + \frac{M}{2}} $$

(d) Expression for Tension ($T$)
From part (b), we established that torque $\tau = TR = I\alpha$. Thus $T = \frac{I\alpha}{R}$.
Substitute the expression for $\alpha$ found in part (b):
$$ T = \frac{I}{R} [ \frac{mg}{R(m + \frac{M}{2})} ] $$
Substitute $I = \frac{1}{2}MR^2$:
$$ T = \frac{\frac{1}{2}MR^2}{R} [ \frac{mg}{R(m + \frac{M}{2})} ] $$
$$ T = \frac{1}{2}MR [ \frac{mg}{R(m + \frac{M}{2})} ] $$
The remaining $R$ terms cancel out:
$$ T = \frac{\frac{1}{2}Mmg}{m + \frac{M}{2}} $$
To clean this up, multiply the numerator and denominator by 2:
Answer:
$$ T = \frac{Mmg}{2m + M} $$

(e) Find the speed with which the object hits the floor
First, we calculate the numerical value of the acceleration $a$ derived in part (c).
Given:
    $m = 5.10 \text{kg}$
    $M = 3.00 \text{kg}$
    $g = 9.80 \text{m/s}^2$
    $h = 6.00 \text{m}$ (displacement)
Calculate acceleration:
$$ a = \frac{mg}{m + \frac{M}{2}} = \frac{(5.10)(9.80)}{5.10 + \frac{3.00}{2}} $$
$$ a = \frac{49.98}{5.10 + 1.50} = \frac{49.98}{6.60} \approx 7.573 \text{m/s}^2 $$
Calculate final speed:
Using the kinematic equation for constant acceleration (starting from rest, $v_i=0$):
$$ v_f^2 = v_i^2 + 2a\Delta y $$
$$ v_f = \sqrt{2ah} $$
$$ v_f = \sqrt{2(7.573)(6.00)} $$
$$ v_f = \sqrt{90.876} $$
$$ v_f \approx 9.53 \text{m/s} $$
Answer: The object hits the floor at $9.53 \text{m/s}$.

(f) Verify your answer using Conservation of Energy
We compare the initial mechanical energy (top) to the final mechanical energy (bottom). The system is isolated.
Initial Energy ($E_i$):
The system starts from rest, so kinetic energy is zero. We set the potential energy reference ($y=0$) at the floor.
$$ E_i = U_g + K = mgh + 0 $$
Final Energy ($E_f$):
The mass is at the floor ($U_g = 0$) and moving with velocity $v$. The reel is rotating with angular velocity $\omega = v/R$.
$$ E_f = K_{trans} + K_{rot} $$
$$ E_f = \frac{1}{2}mv^2 + \frac{1}{2}I\omega^2 $$
Substitute $I = \frac{1}{2}MR^2$ and $\omega = v/R$:
$$ E_f = \frac{1}{2}mv^2 + \frac{1}{2}(\frac{1}{2}MR^2)(\frac{v}{R})^2 $$
$$ E_f = \frac{1}{2}mv^2 + \frac{1}{4}Mv^2 $$
$$ E_f = \frac{1}{2}v^2 (m + \frac{M}{2}) $$
Conservation of Energy ($E_i = E_f$):
$$ mgh = \frac{1}{2}v^2 (m + \frac{M}{2}) $$
Solve for $v$:
$$ v^2 = \frac{2mgh}{m + \frac{M}{2}} $$
$$ v = \sqrt{\frac{2mgh}{m + 0.5M}} $$
Numerical Verification:
$$ v = \sqrt{\frac{2(5.10)(9.80)(6.00)}{5.10 + 1.50}} $$
$$ v = \sqrt{\frac{599.76}{6.60}} $$
$$ v = \sqrt{90.8727...} $$
$$ v \approx 9.53  \text{m/s} $$
Conclusion: The result from the energy method ($9.53 \text{m/s}$) matches the result from the kinematic/force method ($9.53 \text{m/s}$).
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

	// 7. Cleanup
	tex_renderer_destroy(renderer);
	if (layout)
		tex_free(layout);
	free(input_buffer); // Free buffer AFTER layout is freed
	gfx_End();

	return 0;
}
