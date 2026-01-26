#ifndef MATRIX_STEPS_H
#define MATRIX_STEPS_H

#include "app.h"

#include <stdbool.h>
#include <stdint.h>

void steps_clear(StepsLog* log);
bool steps_begin(StepsLog* log, OperationId op);

// adds a step that displays state as a LaTeX matrix
// copies caption/latex into heap memory owned by log
bool steps_append_matrix(StepsLog* log, const char* caption, const MatrixSlot* state);

// adds a step with a LaTeX payload
bool steps_append_tex(StepsLog* log, const char* caption, const char* latex);

#endif // MATRIX_STEPS_H
