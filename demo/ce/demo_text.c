// TI-84+ CE Demo — Simplified TeX Renderer
// Single file, vertical scrolling, direct font rendering.

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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#endif

static const char* kText[] = {
	"Test 1.1: $abc$ (expect: one text node)\n"
	// 1.2: Longer ru
	"Test 1.2: $abcdefghij$ (expect: one text node)\n"
	// 1.3: Multiple separated run
	"Test 1.3: $abc + def$ (expect: N_TEXT(abc), N_GLYPH(+), N_TEXT(def))\n"
	// 1.4: Numbers consolidate to
	"Test 1.4: $12345$ (expect: one text node)\n"
	// 1.5: Mixed letters and number
	"Test 1.5: $abc123xyz$ (expect: one text node)\n"
	// =========================================================================
	// GROUP 2: Script expressions (consolidation limited by scripts)
	// =========================================================================

	// 2.1: Single script - no consolidation possibl
	"Test 2.1: $x^2$ (base must be single glyph)\n"
	// 2.2: Script with preceding tex
	"Test 2.2: $abcx^2$ (expect: N_TEXT(abc), N_SCRIPT(x,2))\n"
	// 2.3: Script with following tex
	"Test 2.3: $x^2abc$ (expect: N_SCRIPT(x,2), N_TEXT(abc))\n"
	// 2.4: Multiple script
	"Test 2.4: $x^2 + y^2$ (two separate scripts)\n"
	// 2.5: Consecutive script
	"Test 2.5: $a^1b^2c^3$ (three scripts, no consolidation)\n"
	// 2.6: Subscrip
	"Test 2.6: $x_1 + y_2$ (subscripts)\n"
	// 2.7: Combined sub/superscrip
	"Test 2.7: $x_1^2$ (both sub and super)\n"
	// 2.8: Text before and after scrip
	"Test 2.8: $abc x^2 def$ (expect: N_TEXT(abc), space, script, space, N_TEXT(def))\n"

	// =========================================================================
	// GROUP 3: Fractions (consolidation within num/den)
	// =========================================================================
	// 3.1: Simple fraction with tex
	"Test 3.1: $\\frac{abc}{xyz}$ (text in both num and den)\n"
	// 3.2: Fraction with scripts in numerato
	"Test 3.2: $\\frac{x^2 + y^2}{abc}$ (scripts in num, text in den)\n"
	// 3.3: Nested fractio
	"Test 3.3: $\\frac{1}{1+\\frac{abc}{def}}$ (nested with text)\n"
	// 3.4: Complex fractio
	"Test 3.4: $\\frac{abcd + efgh}{ijkl - mnop}$ (longer text runs)\n"

	// =========================================================================
	// GROUP 4: Square roots (consolidation within radicand)
	// =========================================================================

	// 4.1: Simple sqrt with tex
	"Test 4.1: $\\sqrt{abc}$ (text under radical)\n"
	// 4.2: Sqrt with script insid
	"Test 4.2: $\\sqrt{x^2 + y^2}$ (scripts under radical)\n"
	// 4.3: Nth root with tex
	"Test 4.3: $\\sqrt[3]{abc}$ (cube root with text)\n"
	// 4.4: Sqrt with text inde
	"Test 4.4: $\\sqrt[abc]{xyz}$ (text in index and radicand)\n"

	// =========================================================================
	// GROUP 5: Grouped expressions (braces)
	// =========================================================================

	// 5.1: Simple group
	"Test 5.1: ${abc}$ (braced text)\n"
	// 5.2: Group as script argumen
	"Test 5.2: $x^{abc}$ (text in superscript group)\n"
	// 5.3: Group as subscript argumen
	"Test 5.3: $x_{abc}$ (text in subscript group)\n"
	// 5.4: Nested group
	"Test 5.4: ${{abc}{def}}$ (nested groups with text)\n"
	// 5.5: Group with scripts insid
	"Test 5.5: ${a^2 b^2}$ (scripts inside group)\n"

	// =========================================================================
	// GROUP 6: Greek letters and symbols (high-byte glyphs)
	// =========================================================================

	// 6.1: Single Greek
	"Test 6.1: $\\alpha$ (single symbol)\n"

	// 6.2: Greek followed by text
	"Test 6.2: $\\alpha abc$ (symbol then text)\n"

	// 6.3: Text followed by Greek
	"Test 6.3: $abc \\alpha$ (text then symbol)\n"

	// 6.4: Multiple Greeks (cannot consolidate - different source positions)
	"Test 6.4: $\\alpha\\beta\\gamma$ (consecutive symbols)\n"

	// 6.5: Greek with script
	"Test 6.5: $\\alpha^2$ (symbol with superscript)\n"

	// 6.6: Mixed Greek and Latin
	"Test 6.6: $\\alpha + \\beta = \\gamma$ (symbols with operators)\n"

	// =========================================================================
	// GROUP 7: Operators and special characters
	// =========================================================================

	// 7.1: Plus/minus between run
	"Test 7.1: $abc + def - ghi$ (operators separate runs)\n"
	// 7.2: Multiplicatio
	"Test 7.2: $abc \\times def$ (times operator)\n"
	// 7.3: Equal
	"Test 7.3: $abc = def$ (equals)\n"
	// 7.4: Parenthese
	"Test 7.4: $(abc)(def)$ (parenthesized groups)\n"
	// 7.5: Bracket
	"Test 7.5: $[abc]$ (bracketed text)\n"

	// =========================================================================
	// GROUP 8: Big operators with limits
	// =========================================================================

	// 8.1: Summation with text limits
	"Test 8.1: $\\sum_{abc}^{xyz}$ (text in sum limits)\n"

	// 8.2: Integral with text limits
	"Test 8.2: $\\int_{abc}^{xyz}$ (text in integral limits)\n"

	// 8.3: Product with text
	"Test 8.3: $\\prod_{i}^{n} abc$ (product followed by text)\n"

	// =========================================================================
	// GROUP 9: Accents and decorations
	// =========================================================================

	// 9.1: Hat over single char
	"Test 9.1: $\\hat{x}$ (hat accent)\n"

	// 9.2: Bar over text (if supported)
	"Test 9.2: $\\bar{abc}$ (bar over text)\n"

	// 9.3: Vec over char
	"Test 9.3: $\\vec{v}$ (vector)\n"

	// 9.4: Overbrace with text
	"Test 9.4: $\\overbrace{abcdef}^{text}$ (overbrace)\n"

	// 9.5: Underbrace with text
	"Test 9.5: $\\underbrace{abcdef}_{text}$ (underbrace)\n"

	// =========================================================================
	// GROUP 10: Functions
	// =========================================================================

	// 10.1: Trig with argument
	"Test 10.1: $\\sin abc$ (function with text arg)\n"

	// 10.2: Log with subscript
	"Test 10.2: $\\log_{abc} xyz$ (log with text base and arg)\n"

	// 10.3: Lim with text
	"Test 10.3: $\\lim_{x \\to abc} xyz$ (limit with text)\n"

	// =========================================================================
	// GROUP 11: Complex expressions (integration tests)
	// =========================================================================

	// 11.1: Quadratic formula components
	"Test 11.1: $\\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}$ (quadratic formula)\n"

	// 11.2: Physics equation
	"Test 11.2: $E = mc^2$ (energy equation)\n"

	// 11.3: Long polynomial
	"Test 11.3: $ax^3 + bx^2 + cx + d$ (polynomial)\n"

	// 11.4: Summation with complex body
	"Test 11.4: $\\sum_{i=1}^{n} (abc_i + def_i)$ (summation)\n"

	// 11.5: Nested fractions with text
	"Test 11.5: $\\frac{abc + \\frac{def}{ghi}}{jkl + mno}$ (nested fractions)\n"

	// 11.6: Multiple terms
	"Test 11.6: $abc + def + ghi + jkl + mno + pqr$ (many text runs)\n"

	// =========================================================================
	// GROUP 12: Edge cases
	// =========================================================================

	// 12.1: Empty math
	//"Test 12.1: $$ (empty - should handle gracefully)\n"

	// 12.2: Single character
	"Test 12.2: $x$ (single char - becomes N_GLYPH)\n"

	// 12.3: Two characters
	"Test 12.3: $xy$ (two chars - becomes N_TEXT)\n"

	// 12.4: Spaces in math
	"Test 12.4: $a \\quad b$ (explicit space)\n"

	// 12.5: Script on group
	"Test 12.5: ${abc}^2$ (group with script)\n"

	// 12.6: Double script attempt
	"Test 12.6: $x^2^3$ (should handle gracefully)\n"

	// 12.7: Standalone script markers (edge case)
	"Test 12.7: $^2$ (superscript without base)\n"

	// 12.8: Very long text run
	"Test 12.8: $abcdefghijklmnopqrstuvwxyz$ (26 letters)\n"

	// =========================================================================
	// GROUP 13: Display math
	// =========================================================================

	// 13.1: Display with text
	"Test 13.1:\n$$abcdef + ghijkl = mnopqr$$\n(display math with text runs)\n"

	// 13.2: Display fraction
	"Test 13.2:\n$$\\frac{abcdefgh}{ijklmnop}$$\n(display fraction with text)\n"

	// 13.3: Display with multiple lines worth
	"Test 13.3:\n$$abc + def + ghi + jkl$$\n(display with operators)\n"

	"Here are the step-by-step solutions. Parts (b), (c), and (d) are solved algebraically as requested, while parts "
	"(e) and (f) include the numerical calculations.\n"
	"\n"
	"(a) Free Body Diagrams\n"
	"Although I cannot draw directly on the page, here is the description of the forces for the Free Body Diagrams:\n"
	"1.  For the Hanging Mass ($m$):\n"
	"    Gravity ($F_g = mg$): Pointing vertically downward.\n"
	"    Tension ($T$): Pointing vertically upward along the string.\n"
	"    Note: Since the object accelerates downward, $mg > T$.\n"
	"\n"
	"2.  For the Reel ($M$):\n"
	"    Tension ($T$): Pointing downward, applied tangentially at the rim (radius $R$) on the right side. This "
	"creates the torque.\n"
	"    Force from Axle ($N$ or $F_{axle}$): Pointing upward at the center to support the reel's weight and the "
	"downward tension.\n"
	"    Gravity ($F_g = Mg$): Pointing downward at the center of the reel.\n"
	"\n"
	"(b) Expression for Angular Acceleration ($\\alpha$)\n"
	"We apply Newton's Second Law for rotation to the reel and translation to the mass.\n"
	"1.  Torque on the reel:\n"
	"$$ \\tau_{net} = I\\alpha $$\n"
	"The torque is caused by tension $T$ at radius $R$:\n"
	"$$ TR = I\\alpha \\quad arrow \\quad T = \\frac{I\\alpha}{R} $$\n"
	"\n"
	"2.  Force on the hanging mass:\n"
	"$$ F_{net} = ma $$\n"
	"$$ mg - T = ma $$\n"
	"\n"
	"3.  Relationship between linear and angular acceleration:\n"
	"Since the string does not slip:\n"
	"$$ a = R\\alpha $$\n"
	"\n"
	"4.  Substitute and Solve:\n"
	"Substitute $T$ from the torque equation and $a$ from the geometric constraint into the force equation:\n"
	"$$ mg - ( \\frac{I\\alpha}{R} ) = m(R\\alpha) $$\n"
	"\n"
	"Move $\\alpha$ terms to one side:\n"
	"$$ mg = mR\\alpha + \\frac{I\\alpha}{R} $$\n"
	"$$ mg = \\alpha ( mR + \\frac{I}{R} ) $$\n"
	"\n"
	"Solve for $\\alpha$:\n"
	"$$ \\alpha = \\frac{mg}{mR + \\frac{I}{R}} $$\n"
	"\n"
	"Substitute the moment of inertia for a solid disk $I = \\frac{1}{2}MR^2$:\n"
	"$$ \\alpha = \\frac{mg}{mR + \\frac{\\frac{1}{2}MR^2}{R}} = \\frac{mg}{mR + \\frac{1}{2}MR} $$\n"
	"$$ \\alpha = \\frac{mg}{R(m + \\frac{1}{2}M)} $$\n"
	"\n"
	"Answer:\n"
	"$$ \\alpha = \\frac{mg}{R(m + \\frac{M}{2})} $$\n"
	"\n"
	"(c) Expression for Translational Acceleration ($a$)\n"
	"We use the relationship $a = R\\alpha$.\n"
	"\n"
	"$$ a = R [ \\frac{mg}{R(m + \\frac{M}{2})} ] $$\n"
	"\n"
	"The $R$ cancels out:\n"
	"Answer:\n"
	"$$ a = \\frac{mg}{m + \\frac{M}{2}} $$\n"
	"\n"
	"(d) Expression for Tension ($T$)\n"
	"From part (b), we established that torque $\\tau = TR = I\\alpha$. Thus $T = \\frac{I\\alpha}{R}$.\n"
	"Substitute the expression for $\\alpha$ found in part (b):\n"
	"\n"
	"$$ T = \\frac{I}{R} [ \\frac{mg}{R(m + \\frac{M}{2})} ] $$\n"
	"\n"
	"Substitute $I = \\frac{1}{2}MR^2$:\n"
	"\n"
	"$$ T = \\frac{\\frac{1}{2}MR^2}{R} [ \\frac{mg}{R(m + \\frac{M}{2})} ] $$\n"
	"$$ T = \\frac{1}{2}MR [ \\frac{mg}{R(m + \\frac{M}{2})} ] $$\n"
	"\n"
	"The remaining $R$ terms cancel out:\n"
	"$$ T = \\frac{\\frac{1}{2}Mmg}{m + \\frac{M}{2}} $$\n"
	"\n"
	"To clean this up, multiply the numerator and denominator by 2:\n"
	"Answer:\n"
	"$$ T = \\frac{Mmg}{2m + M} $$\n"
	"\n"
	"(e) Find the speed with which the object hits the floor\n"
	"First, we calculate the numerical value of the acceleration $a$ derived in part (c).\n"
	"\n"
	"Given:\n"
	"    $m = 5.10 \\text{kg}$\n"
	"    $M = 3.00 \\text{kg}$\n"
	"    $g = 9.80 \\text{m/s}^2$\n"
	"    $h = 6.00 \\text{m}$ (displacement)\n"
	"\n"
	"Calculate acceleration:\n"
	"$$ a = \\frac{mg}{m + \\frac{M}{2}} = \\frac{(5.10)(9.80)}{5.10 + \\frac{3.00}{2}} $$\n"
	"$$ a = \\frac{49.98}{5.10 + 1.50} = \\frac{49.98}{6.60} \\approx 7.573 \\text{m/s}^2 $$\n"
	"\n"
	"Calculate final speed:\n"
	"Using the kinematic equation for constant acceleration (starting from rest, $v_i=0$):\n"
	"$$ v_f^2 = v_i^2 + 2a\\Delta y $$\n"
	"$$ v_f = \\sqrt{2ah} $$\n"
	"$$ v_f = \\sqrt{2(7.573)(6.00)} $$\n"
	"$$ v_f = \\sqrt{90.876} $$\n"
	"$$ v_f \\approx 9.53 \\text{m/s} $$\n"
	"\n"
	"Answer: The object hits the floor at $9.53 \\text{m/s}$.\n"
	"\n"
	"(f) Verify your answer using Conservation of Energy\n"
	"We compare the initial mechanical energy (top) to the final mechanical energy (bottom). The system is isolated.\n"
	"\n"
	"Initial Energy ($E_i$):\n"
	"The system starts from rest, so kinetic energy is zero. We set the potential energy reference ($y=0$) at the "
	"floor.\n"
	"$$ E_i = U_g + K = mgh + 0 $$\n"
	"\n"
	"Final Energy ($E_f$):\n"
	"The mass is at the floor ($U_g = 0$) and moving with velocity $v$. The reel is rotating with angular velocity "
	"$\\omega = v/R$.\n"
	"$$ E_f = K_{trans} + K_{rot} $$\n"
	"$$ E_f = \\frac{1}{2}mv^2 + \\frac{1}{2}I\\omega^2 $$\n"
	"\n"
	"Substitute $I = \\frac{1}{2}MR^2$ and $\\omega = v/R$:\n"
	"$$ E_f = \\frac{1}{2}mv^2 + \\frac{1}{2}(\\frac{1}{2}MR^2)(\\frac{v}{R})^2 $$\n"
	"$$ E_f = \\frac{1}{2}mv^2 + \\frac{1}{4}Mv^2 $$\n"
	"$$ E_f = \\frac{1}{2}v^2 (m + \\frac{M}{2}) $$\n"
	"\n"
	"Conservation of Energy ($E_i = E_f$):\n"
	"$$ mgh = \\frac{1}{2}v^2 (m + \\frac{M}{2}) $$\n"
	"\n"
	"Solve for $v$:\n"
	"$$ v^2 = \\frac{2mgh}{m + \\frac{M}{2}} $$\n"
	"$$ v = \\sqrt{\\frac{2mgh}{m + 0.5M}} $$\n"
	"\n"
	"Numerical Verification:\n"
	"$$ v = \\sqrt{\\frac{2(5.10)(9.80)(6.00)}{5.10 + 1.50}} $$\n"
	"$$ v = \\sqrt{\\frac{599.76}{6.60}} $$\n"
	"$$ v = \\sqrt{90.8727...} $$\n"
	"$$ v \\approx 9.53  \\text{m/s} $$\n"
	"\n"
	"Conclusion: The result from the energy method ($9.53 \\text{m/s}$) matches the result from the kinematic/force "
	"method ($9.53 \\text{m/s}$).\n"
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

int demo_texts_count(void) { return (int)(sizeof(kText) / sizeof(kText[0])); }
const char* demo_texts_get(int idx)
{
	if (idx < 0 || idx >= demo_texts_count())
		return NULL;
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
	char* input_buffer = (char*)malloc(len + 1);
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
			tex_draw(layout, margin, 0, scroll_y);
		}
		else
		{
			gfx_PrintStringXY("Layout Failed", 10, 10);
		}

		gfx_SwapDraw();
	}

	// 7. Cleanup
	if (layout)
		tex_free(layout);
	free(input_buffer); // Free buffer AFTER layout is freed
	gfx_End();

	return 0;
}
