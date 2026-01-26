#include "app.h"

// Forward declarations (file-local)
void screen_enter_main_menu(App* app);
void screen_exit_main_menu(App* app);
void screen_update_main_menu(App* app, uint8_t key);
void screen_draw_main_menu(App* app);

void screen_enter_matrices_menu(App* app);
void screen_exit_matrices_menu(App* app);
void screen_update_matrices_menu(App* app, uint8_t key);
void screen_draw_matrices_menu(App* app);

void screen_enter_operations_menu(App* app);
void screen_exit_operations_menu(App* app);
void screen_update_operations_menu(App* app, uint8_t key);
void screen_draw_operations_menu(App* app);

void screen_enter_slot_picker(App* app);
void screen_exit_slot_picker(App* app);
void screen_update_slot_picker(App* app, uint8_t key);
void screen_draw_slot_picker(App* app);

void screen_enter_result_view(App* app);
void screen_exit_result_view(App* app);
void screen_update_result_view(App* app, uint8_t key);
void screen_draw_result_view(App* app);

void screen_enter_steps_menu(App* app);
void screen_exit_steps_menu(App* app);
void screen_update_steps_menu(App* app, uint8_t key);
void screen_draw_steps_menu(App* app);

void screen_enter_steps_view(App* app);
void screen_exit_steps_view(App* app);
void screen_update_steps_view(App* app, uint8_t key);
void screen_draw_steps_view(App* app);

void screen_enter_element_picker(App* app);
void screen_exit_element_picker(App* app);
void screen_update_element_picker(App* app, uint8_t key);
void screen_draw_element_picker(App* app);

void screen_enter_element_output(App* app);
void screen_exit_element_output(App* app);
void screen_update_element_output(App* app, uint8_t key);
void screen_draw_element_output(App* app);

void screen_enter_matrix_resize(App* app);
void screen_exit_matrix_resize(App* app);
void screen_update_matrix_resize(App* app, uint8_t key);
void screen_draw_matrix_resize(App* app);

void screen_enter_matrix_editor(App* app);
void screen_exit_matrix_editor(App* app);
void screen_update_matrix_editor(App* app, uint8_t key);
void screen_draw_matrix_editor(App* app);

void screen_enter_stub(App* app);
void screen_exit_stub(App* app);
void screen_update_stub(App* app, uint8_t key);
void screen_draw_stub(App* app);

void screen_enter(App* app, ScreenId id)
{
	switch (id)
	{
		case SCREEN_MAIN_MENU: screen_enter_main_menu(app); return;
		case SCREEN_MATRICES_MENU: screen_enter_matrices_menu(app); return;
		case SCREEN_OPERATIONS_MENU: screen_enter_operations_menu(app); return;
		case SCREEN_SLOT_PICKER: screen_enter_slot_picker(app); return;
		case SCREEN_RESULT_VIEW: screen_enter_result_view(app); return;
		case SCREEN_STEPS_MENU: screen_enter_steps_menu(app); return;
		case SCREEN_STEPS_VIEW: screen_enter_steps_view(app); return;
		case SCREEN_ELEMENT_PICKER: screen_enter_element_picker(app); return;
		case SCREEN_ELEMENT_OUTPUT: screen_enter_element_output(app); return;
		case SCREEN_MATRIX_RESIZE: screen_enter_matrix_resize(app); return;
		case SCREEN_MATRIX_EDITOR: screen_enter_matrix_editor(app); return;
		case SCREEN_STUB: screen_enter_stub(app); return;
	}
}

void screen_exit(App* app, ScreenId id)
{
	switch (id)
	{
		case SCREEN_MAIN_MENU: screen_exit_main_menu(app); return;
		case SCREEN_MATRICES_MENU: screen_exit_matrices_menu(app); return;
		case SCREEN_OPERATIONS_MENU: screen_exit_operations_menu(app); return;
		case SCREEN_SLOT_PICKER: screen_exit_slot_picker(app); return;
		case SCREEN_RESULT_VIEW: screen_exit_result_view(app); return;
		case SCREEN_STEPS_MENU: screen_exit_steps_menu(app); return;
		case SCREEN_STEPS_VIEW: screen_exit_steps_view(app); return;
		case SCREEN_ELEMENT_PICKER: screen_exit_element_picker(app); return;
		case SCREEN_ELEMENT_OUTPUT: screen_exit_element_output(app); return;
		case SCREEN_MATRIX_RESIZE: screen_exit_matrix_resize(app); return;
		case SCREEN_MATRIX_EDITOR: screen_exit_matrix_editor(app); return;
		case SCREEN_STUB: screen_exit_stub(app); return;
	}
}

void screen_update(App* app, ScreenId id, uint8_t key)
{
	switch (id)
	{
		case SCREEN_MAIN_MENU: screen_update_main_menu(app, key); return;
		case SCREEN_MATRICES_MENU: screen_update_matrices_menu(app, key); return;
		case SCREEN_OPERATIONS_MENU: screen_update_operations_menu(app, key); return;
		case SCREEN_SLOT_PICKER: screen_update_slot_picker(app, key); return;
		case SCREEN_RESULT_VIEW: screen_update_result_view(app, key); return;
		case SCREEN_STEPS_MENU: screen_update_steps_menu(app, key); return;
		case SCREEN_STEPS_VIEW: screen_update_steps_view(app, key); return;
		case SCREEN_ELEMENT_PICKER: screen_update_element_picker(app, key); return;
		case SCREEN_ELEMENT_OUTPUT: screen_update_element_output(app, key); return;
		case SCREEN_MATRIX_RESIZE: screen_update_matrix_resize(app, key); return;
		case SCREEN_MATRIX_EDITOR: screen_update_matrix_editor(app, key); return;
		case SCREEN_STUB: screen_update_stub(app, key); return;
	}
}

void screen_draw(App* app, ScreenId id)
{
	switch (id)
	{
		case SCREEN_MAIN_MENU: screen_draw_main_menu(app); return;
		case SCREEN_MATRICES_MENU: screen_draw_matrices_menu(app); return;
		case SCREEN_OPERATIONS_MENU: screen_draw_operations_menu(app); return;
		case SCREEN_SLOT_PICKER: screen_draw_slot_picker(app); return;
		case SCREEN_RESULT_VIEW: screen_draw_result_view(app); return;
		case SCREEN_STEPS_MENU: screen_draw_steps_menu(app); return;
		case SCREEN_STEPS_VIEW: screen_draw_steps_view(app); return;
		case SCREEN_ELEMENT_PICKER: screen_draw_element_picker(app); return;
		case SCREEN_ELEMENT_OUTPUT: screen_draw_element_output(app); return;
		case SCREEN_MATRIX_RESIZE: screen_draw_matrix_resize(app); return;
		case SCREEN_MATRIX_EDITOR: screen_draw_matrix_editor(app); return;
		case SCREEN_STUB: screen_draw_stub(app); return;
	}
}
