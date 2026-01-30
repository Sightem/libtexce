#pragma once

#include <cstddef>
#include <cstdint>

#include "matrix_core/arena.hpp"
#include "matrix_core/error.hpp"
#include "matrix_core/explanation.hpp"
#include "matrix_core/matrix.hpp"
#include "matrix_core/rational.hpp"
#include "matrix_core/row_ops.hpp"

namespace matrix_core
{
struct ExplainOptions
{
	bool enable = false;
	Arena* persist = nullptr; // required when enable==true
};

// indices in these APIs are 0 based (consistent with m.at(r, c))
// the shell/ui can present 1 based indices and convert as needed
//
// memory model:
// - matrix data must outlive any Explanation created from it
// - when opts.enable==true, opts.persist must be a valid long lived arena for the
//   explanation context
// - step rendering requires StepRenderBuffers::scratch to be a valid arena, it is
//   cleared by the renderer on each call
//
Error op_add(In MatrixView a, In MatrixView b, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;
Error op_sub(In MatrixView a, In MatrixView b, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;
Error op_mul(In MatrixView a, In MatrixView b, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;
Error op_transpose(In MatrixView a, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

enum class EchelonKind : std::uint8_t
{
	Ref,
	Rref,
};

Error op_echelon(In MatrixView a, In EchelonKind kind, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

Error op_det(In MatrixView a, InOut Arena& scratch, Out Rational* out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

// determinant of A with column "col" replaced by vector "b" (n x 1), useful for
// cramer's rule step breakdown (Δ_i)
Error op_det_replace_column(In MatrixView a, In MatrixView b, In std::uint8_t col, InOut Arena& scratch, Out Rational* out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

enum class MinorCofactorMode : std::uint8_t
{
	Minor,
	Cofactor,
	Both,
};

// computes a single element minor/cofactor for square A
//
// mode==Minor: minor_out required, cofactor_out optional (ignored)
// mode==Cofactor: cofactor_out required, minor_out optional (ignored)
// mode==Both: both outputs required
//
// when opts.enable==true, explanation provides:
//   A -> submatrix -> row ops -> final value(s)
Error op_minor_cofactor(
	In MatrixView a,
	In std::uint8_t i,
	In std::uint8_t j,
	In MinorCofactorMode mode,
	InOut Arena& scratch,
	Out Rational* minor_out,
	Out Rational* cofactor_out,
	Out Explanation* expl,
	In const ExplainOptions& opts) noexcept;

// cramers rule solve (Ax=b), returning x as an n x 1 matrix. no step breakdown
// is produced here, shell can request Δ and Δ_i explanations via op_det()
// and op_det_replace_column()
Error op_cramer_solve(In MatrixView a, In MatrixView b, InOut Arena& scratch, Out MatrixMutView x_out) noexcept;

// inverse via gauss jordan elimination on the augmented matrix [A | I]
//
// on success, writes A^{-1} into out
// when opts.enable==true, explanation steps render the augmented matrix after
// each row operation
Error op_inverse(In MatrixView a, InOut Arena& scratch, Out MatrixMutView out, Out Explanation* expl, In const ExplainOptions& opts) noexcept;

} // namespace matrix_core
