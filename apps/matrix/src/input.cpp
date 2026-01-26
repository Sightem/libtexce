#include "app.h"

#include <ti/getcsc.h>

uint8_t input_poll_key(void)
{
#ifdef __TICE__
	return os_GetCSC();
#else
	static uint8_t last_key = 0;
	const uint8_t key = os_GetCSC();
	if (key == 0)
	{
		last_key = 0;
		return 0;
	}
	if (key == last_key)
		return 0;
	last_key = key;
	return key;
#endif
}
