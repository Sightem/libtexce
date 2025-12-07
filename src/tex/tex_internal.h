#ifndef TEX_TEX_INTERNAL_H
#define TEX_TEX_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "tex_arena.h"
#include "tex_types.h"

// ==================================
// Debug Configuration
// ==================================
#ifndef TEX_DEBUG
#if defined(NDEBUG)
#define TEX_DEBUG 0
#else
#define TEX_DEBUG 1
#endif
#endif

// ==================================
// tuning constants
// ===================================
#define TEX_VIEWPORT_H 240
#define TEX_BASELINE_GAP 1
#define TEX_RULE_THICKNESS 1
#define TEX_FRAC_XPAD 2
#define TEX_FRAC_YPAD 1
#define TEX_FRAC_OUTER_PAD 1
#define TEX_ACCENT_GAP 1
#define TEX_SQRT_HEAD_XPAD 1
#define TEX_SQRT_INDEX_KERNING (-1) // horizontal overlapping (negative) or spacing (positive)
#define TEX_SQRT_INDEX_YSHIFT 2 // vertical shift for index above radical
#define TEX_SCRIPT_XPAD 1

#define TEX_SCRIPT_XPAD 1
#define TEX_SCRIPT_SUP_RAISE 5
#define TEX_SCRIPT_SUB_LOWER 5
#define TEX_LINE_LEADING 1
#define TEX_MATH_ATOMIC_WRAP 1

#define TEX_AXIS_BIAS_INTEGRAL (-2)
#define TEX_AXIS_BIAS_SUM 0
#define TEX_AXIS_BIAS_PROD 0
#define TEX_BIGOP_OVERLAP 2
#define TEX_MULTIOP_KERN 1


// decorative braces height (vertical extent of the brace shape)
#define TEX_BRACE_HEIGHT 4

#define TEX_PARSE_MAX_DEPTH 32
#define TEX_MAX_TOTAL_HEIGHT 20000

// ==================================
// Node Types
// ==================================
typedef enum
{
	N_ROOT = 0,
	N_LINE,
	N_TEXT,
	N_MATH,
	N_GLYPH,
	N_SPACE,
	N_FRAC,
	N_SQRT,
	N_SCRIPT,
	N_OVERLAY,
	N_SPANDECO,
	N_FUNC_LIM,
	N_MULTIOP
} NodeType;

typedef enum
{
	MULTIOP_INT = 0, // standard integral
	MULTIOP_OINT = 1 // contour integral
} MultiOpType;

typedef enum
{
	ACC_VEC = 1,
	ACC_HAT = 2,
	ACC_BAR = 3,
	ACC_DOT = 4,
	ACC_DDOT = 5,
	ACC_OVERLINE = 6,
	ACC_UNDERLINE = 7
} AccentType;

typedef enum
{
	DECO_OVERLINE = 1,
	DECO_UNDERLINE = 2,
	DECO_OVERBRACE = 3,
	DECO_UNDERBRACE = 4
} DecoType;

typedef struct TexStringView
{
	const char* ptr;
	int len;
} TexStringView;

typedef struct Node
{
	int x, y; // position (y can reach total_height ~10000)
	int w; // width (can exceed screen width 320)
	int asc, desc; // TODO: these can be smaller types?
	uint8_t type; // NodeType (13 values, fits in 4 bits even)
	uint8_t flags;
	struct Node* next;
	struct Node* child;
	union
	{
		TexStringView text;
		uint16_t glyph;
		struct
		{
			int width;
			uint8_t em_mul;
		} space;
		struct
		{
			struct Node *num, *den;
		} frac;
		struct
		{
			struct Node* rad; // radicand (content under the radical)
			struct Node* index; // optional root index (NULL for square root)
		} sqrt;
		struct
		{
			struct Node *base, *sub, *sup;
		} script;
		struct
		{
			struct Node* base;
			uint8_t type;
		} overlay;
		struct
		{
			struct Node* content;
			struct Node* label; // optional
			uint8_t deco_type; // DecoType
		} spandeco;
		struct
		{
			struct Node* limit;
		} func_lim;
		struct
		{
			uint8_t count; // number of operators (2 for \iint, 3 for \iiint, etc)
			uint8_t op_type; // MultiOpType
		} multiop;

	} data;
} Node;

// =======================================
// Line Structure
// =======================================
typedef struct TeX_Line
{
	int y;
	int h;
	Node* first;
	int child_count;
	struct TeX_Line* next;
} TeX_Line;

// =======================================
// Error State
// =======================================
typedef struct
{
	int code; // TeX_Error value
	const char* msg; // Static string (ROM)
	int val; // Detail value
#if TEX_DEBUG
	const char* file;
	int line;
#endif
} TexErrorState;

// ==================================
// Layout Structure
// ==================================
typedef struct TeX_Layout
{
	// Config (copied from TeX_Config at format time)
	struct
	{
		uint8_t fg, bg;
		const char* pack;
		TeX_ErrorLogFn error_callback;
		void* error_userdata;
	} cfg;

	int width;
	int total_height;
	TeX_Line* lines;
	TeX_Line** line_index;
	int line_count;
	TexArena arena;
	Node* root;

	// Error state
	TexErrorState error;


#if TEX_DEBUG
	unsigned debug_flags;
#endif
} TeX_Layout;


// =======================================
// debug / recorder mode structures
// =======================================
typedef enum
{
	DOP_TEXT = 1,
	DOP_GLYPH,
	DOP_RULE,
	DOP_LINE,
	DOP_DOT,
	DOP_ELLIPSE
} TexDrawOpType;

typedef struct
{
	TexDrawOpType type;
	int x1, y1, x2, y2;
	int w, h;
	int glyph;
	const char* text;
	int text_len;
	int role;
} TexDrawOp;

// recorder functions (nops in direct mode)
void tex_draw_log_reset(void);
int tex_draw_log_count(void);
int tex_draw_log_get(TexDrawOp* out, int max);
int tex_draw_log_get_range(TexDrawOp* out, int start_index, int count);


// =======================================
// error handling macros
// =======================================

#define TEX_INVOKE_CALLBACK_(L, LVL, MSG, FILE, LINE)                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((L) && (L)->cfg.error_callback)                                                                            \
		{                                                                                                              \
			(L)->cfg.error_callback((L)->cfg.error_userdata, (LVL), (MSG), (FILE), (LINE));                            \
		}                                                                                                              \
	}                                                                                                                  \
	while (0)

#if TEX_DEBUG
#define TEX_SET_ERROR(layout, ecode, emsg, eval)                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((layout) && (layout)->error.code == TEX_OK)                                                                \
		{                                                                                                              \
			(layout)->error.code = (ecode);                                                                            \
			(layout)->error.msg = (emsg);                                                                              \
			(layout)->error.val = (eval);                                                                              \
			(layout)->error.file = __FILE__;                                                                           \
			(layout)->error.line = __LINE__;                                                                           \
			TEX_INVOKE_CALLBACK_((layout), 2, (emsg), __FILE__, __LINE__);                                             \
		}                                                                                                              \
	}                                                                                                                  \
	while (0)

#define TEX_SET_WARNING(layout, wmsg)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((layout))                                                                                                  \
		{                                                                                                              \
			TEX_INVOKE_CALLBACK_((layout), 1, (wmsg), __FILE__, __LINE__);                                             \
		}                                                                                                              \
	}                                                                                                                  \
	while (0)
#else
#define TEX_SET_ERROR(layout, ecode, emsg, eval)                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((layout) && (layout)->error.code == TEX_OK)                                                                \
		{                                                                                                              \
			(layout)->error.code = (ecode);                                                                            \
			(layout)->error.msg = (emsg);                                                                              \
			(layout)->error.val = (eval);                                                                              \
			TEX_INVOKE_CALLBACK_((layout), 2, (emsg), NULL, 0);                                                        \
		}                                                                                                              \
	}                                                                                                                  \
	while (0)

#define TEX_SET_WARNING(layout, wmsg)                                                                                  \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((layout))                                                                                                  \
		{                                                                                                              \
			TEX_INVOKE_CALLBACK_((layout), 1, (wmsg), NULL, 0);                                                        \
		}                                                                                                              \
	}                                                                                                                  \
	while (0)
#endif

#define TEX_HAS_ERROR(layout) ((layout) && (layout)->error.code != TEX_OK)

#define TEX_CLEAR_ERROR(layout)                                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((layout))                                                                                                  \
		{                                                                                                              \
			memset(&(layout)->error, 0, sizeof((layout)->error));                                                      \
		}                                                                                                              \
	}                                                                                                                  \
	while (0)

#endif // TEX_TEX_INTERNAL_H
