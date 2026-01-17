/**
 * test_harness.h - Shared test harness for libtexce autotests
 * 
 * Usage:
 *   #include "test_harness.h"
 *   int main(void) {
 *       TEST_INIT();
 *       TEST_RENDER("$\\frac{a}{b}$");
 *       TEST_WAIT_KEY();
 *       TEST_CLEANUP();
 *       return 0;
 *   }
 */

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fontlibc.h>
#include <graphx.h>
#include <ti/getcsc.h>
#include <tice.h>

#include "tex/tex.h"

#define TEST_COL_BG 255
#define TEST_COL_FG 0
#define TEST_WIDTH 300

static fontlib_font_t *g_test_font_main = NULL;
static fontlib_font_t *g_test_font_script = NULL;
static TeX_Renderer *g_test_renderer = NULL;

/**
 * Initialize graphics and fonts. Returns 1 on error.
 */
static inline int test_harness_init(void)
{
    gfx_Begin();
    gfx_FillScreen(TEST_COL_BG);

    g_test_font_main = fontlib_GetFontByIndex("TeXFonts", 0);
    g_test_font_script = fontlib_GetFontByIndex("TeXScrpt", 0);

    if (!g_test_font_main || !g_test_font_script)
    {
        gfx_SetTextFGColor(TEST_COL_FG);
        gfx_PrintStringXY("Font Error", 10, 10);
        while (!os_GetCSC())
            ;
        gfx_End();
        return 1;
    }

    tex_draw_set_fonts(g_test_font_main, g_test_font_script);
    fontlib_SetTransparency(true);
    fontlib_SetForegroundColor(TEST_COL_FG);
    fontlib_SetBackgroundColor(TEST_COL_BG);

    g_test_renderer = tex_renderer_create();
    if (!g_test_renderer)
    {
        gfx_SetTextFGColor(TEST_COL_FG);
        gfx_PrintStringXY("Renderer Error", 10, 10);
        while (!os_GetCSC())
            ;
        gfx_End();
        return 1;
    }

    return 0;
}

/**
 * render a LaTeX expression at the given position
 * returns the vertical position for the next expression
 */
static inline int test_harness_render(const char *expr, int x, int y)
{
    // Copy to mutable buffer since tex_format modifies in place
    size_t len = strlen(expr);
    char *buf = (char *)malloc(len + 1);
    if (!buf)
        return y;
    strcpy(buf, expr);

    TeX_Config cfg = {
        .color_fg = TEST_COL_FG,
        .color_bg = TEST_COL_BG,
        .font_pack = "TeXFonts"
    };

    TeX_Layout *layout = tex_format(buf, TEST_WIDTH, &cfg);
    if (layout)
    {
        tex_draw(g_test_renderer, layout, x, y, 0);
        y += tex_get_total_height(layout) + 5;
        tex_free(layout);
    }
    free(buf);
    return y;
}

static inline void test_harness_cleanup(void)
{
    if (g_test_renderer)
        tex_renderer_destroy(g_test_renderer);
    gfx_End();
}

#define TEST_INIT() do { if (test_harness_init()) return 1; gfx_FillScreen(TEST_COL_BG); } while(0)
#define TEST_RENDER(expr, x, y) test_harness_render((expr), (x), (y))
#define TEST_WAIT_KEY() do { while (!os_GetCSC()); } while(0)
#define TEST_CLEANUP() test_harness_cleanup()

#endif /* TEST_HARNESS_H */
