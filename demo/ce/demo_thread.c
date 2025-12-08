#include <fontlibc.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tice.h>

#include "tex/tex.h"

// Define roles
typedef enum
{
	ROLE_USER,
	ROLE_ASSISTANT
} ChatRole;

typedef struct
{
	ChatRole role;
	TeX_Layout* layout;
	char* source_buffer; // <--- FIX: Keep reference to the buffer
	int width;
	int height;
	int y_pos;
} ChatMessage;

#define MAX_MESSAGES 10
ChatMessage thread[MAX_MESSAGES];
int msg_count = 0;
int total_thread_height = 20;

void add_message(const char* text, ChatRole role, int screen_width, TeX_Config* cfg)
{
	if (msg_count >= MAX_MESSAGES)
		return;

	// 1. Allocate buffer and KEEP it (do not free until end of program)
	char* buf = (char*)malloc(strlen(text) + 1);
	if (!buf)
		return;
	strcpy(buf, text);

	// 2. Format
	int bubble_width = (role == ROLE_USER) ? (screen_width - 40) : screen_width;
	TeX_Layout* l = tex_format(buf, bubble_width, cfg);

	// 3. Store
	ChatMessage* msg = &thread[msg_count++];
	msg->role = role;
	msg->layout = l;
	msg->source_buffer = buf; // Store pointer so we can free it later
	msg->width = bubble_width;
	msg->height = tex_get_total_height(l) + 14;

	// 4. Stack
	msg->y_pos = total_thread_height;
	total_thread_height += msg->height + 20;
}

int main(void)
{
	gfx_Begin();
	gfx_SetDrawBuffer();

	// Load Fonts
	fontlib_font_t* f_main = fontlib_GetFontByIndex("TeXFonts", 0);
	fontlib_font_t* f_script = fontlib_GetFontByIndex("TeXScrpt", 0);
	if (!f_main || !f_script)
	{
		gfx_End();
		return 1;
	}

	tex_draw_set_fonts(f_main, f_script);
	fontlib_SetTransparency(true);

	// Create one shared renderer for all messages (stateless mode)
	TeX_Renderer* renderer = tex_renderer_create();
	if (!renderer)
	{
		gfx_End();
		return 1;
	}

	TeX_Config cfg = { .color_fg = 0, .color_bg = 255, .font_pack = "TeXFonts" };

	const int screen_w = GFX_LCD_WIDTH - 20;

	// --- Build Chat ---
	add_message("Hello! Can you help me with a physics problem?", ROLE_USER, screen_w, &cfg);

	add_message("Certainly. I can help you calculate properties of mass distributions. "
	            "For example, the Center of Mass is defined as:\n"
	            "$$x_{cm} = \\frac{1}{M} \\int x \\lambda(x) dx$$",
	            ROLE_ASSISTANT, screen_w, &cfg);

	add_message("What if the density $\\lambda(x)$ is constant?", ROLE_USER, screen_w, &cfg);

	add_message("If $\\lambda$ is constant, it factors out:\n"
	            "$$x_{cm} = \\frac{\\lambda}{M} [ \\frac{1}{2}x^2 ]_0^L = \\frac{L}{2}$$",
	            ROLE_ASSISTANT, screen_w, &cfg);

	int scroll_y = 0;

	while (true)
	{
		kb_Scan();
		if (kb_Data[6] & kb_Clear)
			break;

		// Arrow keys are in group 7
		kb_key_t arrows = kb_Data[7];

		if (arrows & kb_Up)
			scroll_y -= 10;
		if (arrows & kb_Down)
			scroll_y += 10;

		// Clamp scroll AFTER input
		int view_h = GFX_LCD_HEIGHT;
		int max_scroll = total_thread_height - view_h;
		if (max_scroll < 0)
			max_scroll = 0;

		if (scroll_y < 0)
			scroll_y = 0;
		if (scroll_y > max_scroll)
			scroll_y = max_scroll;


		gfx_FillScreen(255);

		for (int i = 0; i < msg_count; i++)
		{
			ChatMessage* m = &thread[i];

			// FIX 2: Calculate absolute screen Y.
			// We do the math here so we have full control.
			int screen_y = m->y_pos - scroll_y;

			// Culling: Check if message is visible
			if (screen_y + m->height < 0 || screen_y > GFX_LCD_HEIGHT)
				continue;

			int draw_x = 10; // Left align all messages

			// FIX 3: Draw Header with Safety Check
			// header_y is where the text starts
			int header_y = screen_y; // Header starts at message top
			int content_y = screen_y + 14; // Content below header

			// Draw header only if on-screen
			if (header_y >= 0 && header_y < GFX_LCD_HEIGHT)
			{
				gfx_SetTextFGColor(0);
				gfx_SetTextXY(draw_x, header_y);
				gfx_PrintString(m->role == ROLE_USER ? "User:" : "Assistant:");
			}

			gfx_SetColor(0);

			// Draw TeX content
			tex_draw(renderer, m->layout, draw_x, content_y, 0);
		}


		// Scrollbar
		if (total_thread_height > view_h)
		{
			int bar_h = (view_h * view_h) / total_thread_height;
			if (bar_h < 10)
				bar_h = 10;
			int bar_y = (scroll_y * (view_h - bar_h)) / max_scroll;
			gfx_SetColor(200);
			gfx_FillRectangle(GFX_LCD_WIDTH - 4, bar_y, 4, bar_h);
		}
		gfx_SwapDraw();
	}

	// Cleanup: Free renderer, layouts, and source buffers
	tex_renderer_destroy(renderer);
	for (int i = 0; i < msg_count; i++)
	{
		tex_free(thread[i].layout);
		free(thread[i].source_buffer); // <--- Memory safety restored
	}
	gfx_End();
	return 0;
}
