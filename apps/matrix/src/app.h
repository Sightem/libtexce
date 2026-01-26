#ifndef MATRIX_APP_H
#define MATRIX_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


enum { MATRIX_SLOTS = 4 };
enum { MATRIX_MAX_ROWS = 6 };
enum { MATRIX_MAX_COLS = 6 };
enum { NAV_STACK_MAX = 16 };
enum { MATRIX_STEPS_MAX = 96 };

enum {
	APP_HEADER_TEXT_Y = 6,
	APP_HEADER_LINE_Y = 18,
	APP_CONTENT_TOP_Y = 20,
	APP_FOOTER_LINE_Y = 222,
	APP_FOOTER_TEXT_Y = 230,
};

typedef struct TeX_Layout TeX_Layout;
typedef struct TeX_Renderer TeX_Renderer;
typedef struct StepsLog StepsLog;

typedef struct Rational {
	int32_t num;
	int32_t den;
} Rational;

Rational rational_from_int(int32_t value);
Rational rational_add(Rational a, Rational b);
Rational rational_sub(Rational a, Rational b);
Rational rational_mul(Rational a, Rational b);
Rational rational_div(Rational a, Rational b);
Rational rational_neg(Rational a);
bool rational_is_zero(Rational a);

typedef struct MatrixSlot {
	uint8_t rows; // 0 means "unset"
	uint8_t cols; // 0 means "unset"
	Rational cell[MATRIX_MAX_ROWS][MATRIX_MAX_COLS];
} MatrixSlot;

bool matrix_is_set(const MatrixSlot* m);
void matrix_clear(MatrixSlot* m);
bool matrix_add(const MatrixSlot* a, const MatrixSlot* b, MatrixSlot* out);
bool matrix_sub(const MatrixSlot* a, const MatrixSlot* b, MatrixSlot* out);
bool matrix_mul(const MatrixSlot* left, const MatrixSlot* right, MatrixSlot* out);
bool matrix_ref(const MatrixSlot* in, MatrixSlot* out);
bool matrix_rref(const MatrixSlot* in, MatrixSlot* out);
bool matrix_ref_steps(const MatrixSlot* in, MatrixSlot* out, StepsLog* steps);
bool matrix_rref_steps(const MatrixSlot* in, MatrixSlot* out, StepsLog* steps);
bool matrix_det(const MatrixSlot* in, Rational* out_det);
bool matrix_det_steps(const MatrixSlot* in, Rational* out_det, StepsLog* steps);

typedef enum CramerStatus {
	CRAMER_OK = 0,
	CRAMER_INVALID,
	CRAMER_SINGULAR,
} CramerStatus;

CramerStatus matrix_cramer(const MatrixSlot* a, const MatrixSlot* b, MatrixSlot* out);

typedef enum ScreenId {
	SCREEN_MAIN_MENU = 0,
	SCREEN_MATRICES_MENU,
	SCREEN_OPERATIONS_MENU,
	SCREEN_SLOT_PICKER,
	SCREEN_RESULT_VIEW,
	SCREEN_STEPS_MENU,
	SCREEN_STEPS_VIEW,
	SCREEN_ELEMENT_PICKER,
	SCREEN_ELEMENT_OUTPUT,
	SCREEN_MATRIX_RESIZE,
	SCREEN_MATRIX_EDITOR,
	SCREEN_STUB,
} ScreenId;

typedef enum OperationId {
	OP_NONE = 0,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_REF,
	OP_RREF,
	OP_DET,
	OP_ELEMENT, // minor/cofactor element query
	OP_CRAMER,
} OperationId;

typedef struct OpContext {
	OperationId op;
	uint8_t lhs_slot;
	uint8_t rhs_slot;
} OpContext;

typedef enum SlotPickerPurpose {
	PICKER_NONE = 0,
	PICKER_CLEAR_SLOT,
	PICKER_OP_LHS,
	PICKER_OP_RHS,
	PICKER_SAVE_RESULT,
} SlotPickerPurpose;

typedef struct SlotPickerState {
	SlotPickerPurpose purpose;
	OperationId op;
	uint8_t selected;
	const char* title;
	bool require_set;
} SlotPickerState;

typedef struct AppMenuState {
	uint8_t selected;
} AppMenuState;

typedef struct MatrixResizeState {
	uint8_t slot;
	uint8_t rows;
	uint8_t cols;
} MatrixResizeState;

typedef struct MatrixEditorState {
	uint8_t slot;
	uint8_t cursor_row;
	uint8_t cursor_col;

	bool editing;
	char edit_buf[16];
	uint8_t edit_len;
} MatrixEditorState;

typedef struct ResultViewState {
	uint8_t cursor_row;
	uint8_t cursor_col;
} ResultViewState;

typedef struct StepItem {
	char* caption;
	char* latex;	
} StepItem;

typedef struct StepsLog {
	OperationId op;
	uint8_t count;
	bool has_steps;
	bool truncated;
	StepItem items[MATRIX_STEPS_MAX];
} StepsLog;

typedef struct StepsViewState {
	uint8_t index;
	bool pending_2nd;
	TeX_Layout* layout;
	char* layout_buf;
} StepsViewState;

typedef struct StepsMenuState {
	uint8_t selected;
	uint8_t count;
	const char* items[8];
	char labels[8][20];
} StepsMenuState;

typedef struct LastOpInfo {
	OperationId op;
	uint8_t lhs_slot;
	uint8_t rhs_slot;
	uint8_t i; // 1-based (element ops)
	uint8_t j; // 1-based (element ops)
} LastOpInfo;

typedef enum ElementOutMode {
	ELEM_OUT_MINOR = 0,
	ELEM_OUT_COFACTOR,
	ELEM_OUT_BOTH,
} ElementOutMode;

typedef struct ElementQueryState {
	uint8_t slot;
	uint8_t n;
	uint8_t i; // 1-based
	uint8_t j; // 1-based
	ElementOutMode mode;
	uint8_t out_menu_selected;
} ElementQueryState;

typedef struct App {
	MatrixSlot matrices[MATRIX_SLOTS];
	MatrixSlot last_result;
	bool has_last_result;
	char last_label[24];
	bool has_last_label;

	ScreenId current;
	ScreenId nav_stack[NAV_STACK_MAX];
	uint8_t nav_depth;
	bool running;

	AppMenuState main_menu;
	AppMenuState matrices_menu;
	AppMenuState operations_menu;
	SlotPickerState slot_picker;
	OpContext op_ctx;
	MatrixResizeState matrix_resize;
	MatrixEditorState matrix_editor;
	ResultViewState result_view;
	StepsMenuState steps_menu;
	ElementQueryState element_query;

	char toast[48];
	bool has_toast;

	TeX_Renderer* tex_renderer;

	LastOpInfo last_op;
	StepsLog last_steps;
	StepsViewState steps_view;

#ifndef NDEBUG
#ifdef __TICE__
	uint32_t dbg_free_ram_bytes;
	uint16_t dbg_frame_counter;
#endif
#endif
} App;

void app_init(App* app);
int app_run(App* app);

void screen_enter(App* app, ScreenId id);
void screen_exit(App* app, ScreenId id);
void screen_update(App* app, ScreenId id, uint8_t key);
void screen_draw(App* app, ScreenId id);

void nav_push(App* app, ScreenId next);
void nav_pop(App* app);
void nav_replace(App* app, ScreenId next);

void app_draw_header(const char* title);
void app_draw_footer(const char* left, const char* right);
void app_draw_toast(App* app);
void app_draw_debug_overlay(const App* app);
uint8_t input_poll_key(void);

typedef struct UiMenu {
	const char* title;
	const char* const* items;
	uint8_t count;
	uint8_t* selected;
} UiMenu;

bool ui_menu_update(UiMenu* menu, uint8_t key, bool* out_back);
void ui_menu_draw(const UiMenu* menu);
char slot_name(uint8_t slot); // 0->'A'

#endif // MATRIX_APP_H
