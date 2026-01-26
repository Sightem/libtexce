#include "app.h"

int main(void)
{
	static App app;
	app_init(&app);
	return app_run(&app);
}

