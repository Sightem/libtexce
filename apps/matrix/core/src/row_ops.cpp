#include "matrix_core/row_ops.hpp"

#include "matrix_core/writer.hpp"

namespace matrix_core
{

ErrorCode row_op_caption(const RowOp& op, char* out, std::size_t cap) noexcept
{
	if (!out || cap == 0)
		return ErrorCode::BufferTooSmall;
	out[0] = '\0';

	Writer w{ out, cap, 0 };

	ErrorCode ec = w.append("R");
	if (!is_ok(ec))
		return ec;
	ec = w.append_index1(op.target_row);
	if (!is_ok(ec))
		return ec;

	switch (op.kind)
	{
		case RowOpKind::Swap:
			ec = w.append(" <-> R");
			if (!is_ok(ec))
				return ec;
			return w.append_index1(op.source_row);
		case RowOpKind::Scale:
			ec = w.append(" <- (");
			if (!is_ok(ec))
				return ec;
			ec = w.append_rational_latex(op.scalar);
			if (!is_ok(ec))
				return ec;
			ec = w.append(") R");
			if (!is_ok(ec))
				return ec;
			return w.append_index1(op.target_row);
		case RowOpKind::AddMul:
			ec = w.append(" <- R");
			if (!is_ok(ec))
				return ec;
			ec = w.append_index1(op.target_row);
			if (!is_ok(ec))
				return ec;
			ec = w.append(" + (");
			if (!is_ok(ec))
				return ec;
			ec = w.append_rational_latex(op.scalar);
			if (!is_ok(ec))
				return ec;
			ec = w.append(") R");
			if (!is_ok(ec))
				return ec;
			return w.append_index1(op.source_row);
	}
	__builtin_unreachable();
}

} // namespace matrix_core
