#ifndef TEX_TEX_INTERNAL_H
#define TEX_TEX_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "tex_pool.h"
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

#define TEX_DELIM_WIDTH_FACTOR 4
#define TEX_DELIM_MIN_WIDTH 4
#define TEX_DELIM_MAX_WIDTH 10

// spacing for matrix layout
#define TEX_MATRIX_COL_SPACING 4
#define TEX_MATRIX_ROW_SPACING 2

// safety limit for stack-allocated metric arrays during measure/draw
#define TEX_MATRIX_MAX_DIMS 16


// decorative braces height (vertical extent of the brace shape)
#define TEX_BRACE_HEIGHT 4

#define TEX_PARSE_MAX_DEPTH 32
#define TEX_MAX_TOTAL_HEIGHT 20000

// Node.flags constants
#define TEX_FLAG_MATHF_DISPLAY 0x01 // Display-mode math (centered)
#define TEX_FLAG_SCRIPT 0x02 // Node measured with script role

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
	N_MULTIOP,
	N_AUTO_DELIM,
	N_MATRIX
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

typedef enum
{
	DELIM_NONE = 0,
	DELIM_PAREN,
	DELIM_BRACKET,
	DELIM_BRACE,
	DELIM_VERT,
	DELIM_ANGLE,
	DELIM_FLOOR,
	DELIM_CEIL
} DelimType;

typedef struct Node
{
	int16_t w; // width
	int16_t asc, desc; // ascender/descender heights
	uint8_t type; // NodeType
	uint8_t flags;
	union
	{
		struct
		{
			StringId sid; // offset to string data in pool
			uint16_t len; // string length
		} text;
		uint16_t glyph;
		struct
		{
			int16_t width;
			uint8_t em_mul;
		} space;
		// N_MATH, N_ROOT, N_LINE: sequence container with ListId
		struct
		{
			ListId head; // first block of child node list
		} list;
		struct
		{
			NodeRef num, den;
		} frac;
		struct
		{
			NodeRef rad; // radicand (content under the radical)
			NodeRef index; // optional root index (NODE_NULL for square root)
		} sqrt;
		struct
		{
			NodeRef base, sub, sup;
		} script;
		struct
		{
			NodeRef base;
			uint8_t type;
		} overlay;
		struct
		{
			NodeRef content;
			NodeRef label; // optional (NODE_NULL if none)
			uint8_t deco_type; // DecoType
		} spandeco;
		struct
		{
			NodeRef limit;
		} func_lim;
		struct
		{
			uint8_t count; // number of operators (2 for \iint, etc)
			uint8_t op_type; // MultiOpType
		} multiop;
		struct
		{
			ListId content; // ListId instead of NodeRef
			uint8_t left_type; // DelimType
			uint8_t right_type; // DelimType
			int16_t delim_h; // cached symmetric height
		} auto_delim;
		struct
		{
			ListId cells; // flat list of all cell nodes in row major order
			uint8_t rows; // number of rows
			uint8_t cols; // number of columns
			uint8_t delim_type; // delim type enum for brackets
		} matrix;
	} data;
} Node;

// =======================================
// Pool Accessors (inline, sizeof(Node) visible)
// =======================================

// reserved nodes for ASCII glyphs (defined in tex_metrics.c)
extern Node g_reserved_nodes[TEX_RESERVED_COUNT];

static inline Node* pool_get_node(UnifiedPool* pool, NodeRef ref)
{
	if (ref == NODE_NULL)
		return NULL;
	// reserved refs map to static flyweight nodes
	if (TEX_IS_RESERVED_REF(ref))
		return &g_reserved_nodes[TEX_RESERVED_INDEX(ref)];
	return (Node*)(pool->slab + ((size_t)ref * sizeof(Node)));
}

static inline const char* pool_get_string(UnifiedPool* pool, StringId id)
{
	if (id == STRING_NULL)
		return "";
	return (const char*)(pool->slab + id);
}

static inline TexListBlock* pool_get_list_block(UnifiedPool* pool, ListId id)
{
	if (id == LIST_NULL)
		return NULL;
	return (TexListBlock*)(pool->slab + id);
}

// =======================================
// Line Structure
// =======================================
typedef struct TeX_Line
{
	int y;
	int h;
	int x_offset; // horizontal offset for centered content (display math)
	ListId content; // ListId for nodes in this line
	int child_count;
	struct TeX_Line* next; // kept for now, will be array in renderer
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
// Checkpoint for Sparse Indexing
// ==================================
#define TEX_CHECKPOINT_INTERVAL 200 // pixels between checkpoints

typedef struct
{
	int y_pos; // Vertical pixel coordinate at line start
	const char* src_ptr; // Pointer into source buffer at this line
} TeX_Checkpoint;

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

	// source buffer pointer (immutable after format)
	const char* source;

	TeX_Checkpoint* checkpoints;
	int checkpoint_count;
	int checkpoint_capacity;

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
