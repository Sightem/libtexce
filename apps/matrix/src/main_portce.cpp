#include <tice.h>

#include "app.h"

#include <PortCE.h>

int main(void)
{
	PortCE_initialize("MATRIX");

	static App app;
	app_init(&app);
	const int rc = app_run(&app);

	PortCE_terminate();
	return rc;
}
