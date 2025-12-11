#ifndef TEX_TEX_TYPES_H
#define TEX_TEX_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int v;
} TexCoord;
typedef struct
{
	int v;
} TexBaseline;

// ================================
// error codes
// ================================
typedef enum
{
	TEX_OK = 0,
	TEX_ERR_OOM,
	TEX_ERR_FONT,
	TEX_ERR_PARSE,
	TEX_ERR_INPUT,
	TEX_ERR_DEPTH
} TeX_Error;

// ================================
// error callback
// ================================
// level: 0=info, 1=warning, 2=error
// msg: static string describing the error (may be NULL in release builds)
// file/line: source location (NULL/0 in release builds)
typedef void (*TeX_ErrorLogFn)(void* userdata, int level, const char* msg, const char* file, int line);

// ================================
// configuration
// ================================
typedef struct
{
	uint8_t color_fg;
	uint8_t color_bg;
	const char* font_pack;
	TeX_ErrorLogFn error_callback;
	void* error_userdata;
} TeX_Config;

#ifdef __cplusplus
}
#endif

#endif // TEX_TEX_TYPES_H
