#ifndef TEX_TEX_UTIL_H
#define TEX_TEX_UTIL_H

#include <stdarg.h>
#include <stdio.h>

#ifndef TEX_DEBUG
#define TEX_DEBUG 0
#endif

#if TEX_DEBUG
#if defined(__TICE__)
#include <debug.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
static inline void tex_trace_impl(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buf[128];
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	dbg_printf("[TEX] %s\n", buf);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#define TEX_TRACE(...) tex_trace_impl(__VA_ARGS__)
#else
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
static inline void tex_trace_impl(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[TEX] ");
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#define TEX_TRACE(...) tex_trace_impl(__VA_ARGS__)
#endif
#else
#define TEX_TRACE(fmt, ...)                                                                                            \
	do                                                                                                                 \
	{                                                                                                                  \
	}                                                                                                                  \
	while (0)
#endif

#ifndef TEX_ENABLE_ASSERTS
#if defined(NDEBUG)
#define TEX_ENABLE_ASSERTS 0
#else
#define TEX_ENABLE_ASSERTS 1
#endif
#endif

#if TEX_ENABLE_ASSERTS
#include <assert.h>
#define TEX_ASSERT(cond) assert(cond)
#else
#define TEX_ASSERT(cond)                                                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		(void)sizeof(cond);                                                                                            \
	}                                                                                                                  \
	while (0)
#endif

// Safe coordinate assignment macro for Node fields (x, y, w, asc, desc).
// In debug builds, asserts that the value fits in int16_t range.
// In release builds, just casts directly for zero overhead.
#if TEX_ENABLE_ASSERTS
#define TEX_COORD_ASSIGN(dst, val)                                                                                     \
	do                                                                                                                 \
	{                                                                                                                  \
		int _coord_val = (val);                                                                                        \
		TEX_ASSERT(_coord_val >= -32768 && _coord_val <= 32767 && "Coordinate overflow: value out of int16_t range");  \
		(dst) = (int16_t)_coord_val;                                                                                   \
	}                                                                                                                  \
	while (0)
#else
#define TEX_COORD_ASSIGN(dst, val) ((dst) = (int16_t)(val))
#endif

#define TEX_MIN(a, b) ((a) < (b) ? (a) : (b))
#define TEX_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TEX_ABS(x) (((x) < 0) ? -(x) : (x))
#define TEX_CLAMP(v, lo, hi) (((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v)))


int tex_util_unescaped_len(const char* s, int raw_len);

void tex_util_copy_unescaped(char* dst, const char* s, int raw_len);

int tex_util_is_escape_char(char c);


#endif // TEX_TEX_UTIL_H
