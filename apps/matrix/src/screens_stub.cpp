#include "app.h"

#include <graphx.h>
#include <ti/getcsc.h>

void screen_enter_stub(App* app) { (void)app; }
void screen_exit_stub(App* app) { (void)app; }

void screen_update_stub(App* app, uint8_t key)
{
	if (key == sk_Clear || key == sk_Enter)
		nav_pop(app);
}

void screen_draw_stub(App* app)
{
	app_draw_header("Not implemented");
	gfx_PrintStringXY("This section is not implemented yet.", 10, 60);
	app_draw_footer("CLEAR:Back", "ENTER:Back");
	(void)app;
}
