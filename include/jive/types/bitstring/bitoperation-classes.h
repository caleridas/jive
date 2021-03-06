/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_TYPES_BITSTRING_BITOPERATION_CLASSES_H
#define JIVE_TYPES_BITSTRING_BITOPERATION_CLASSES_H

#include <jive/rvsdg/binary.h>
#include <jive/rvsdg/unary.h>
#include <jive/types/bitstring/type.h>
#include <jive/types/bitstring/value-representation.h>

namespace jive {

/* Represents a unary operation on a bitstring of a specific width,
 * produces another bitstring of the same width. */
class bitunary_op : public jive::unary_op {
public:
	virtual
	~bitunary_op() noexcept;

	inline
	bitunary_op(const bittype & type) noexcept
	: unary_op(type, type)
	{}

	inline const bittype &
	type() const noexcept
	{
		return *static_cast<const bittype*>(&argument(0).type());
	}

	/* reduction methods */
	virtual jive_unop_reduction_path_t
	can_reduce_operand(
		const jive::output * arg) const noexcept override;

	virtual jive::output *
	reduce_operand(
		jive_unop_reduction_path_t path,
		jive::output * arg) const override;

	virtual bitvalue_repr
	reduce_constant(
		const bitvalue_repr & arg) const = 0;

	virtual std::unique_ptr<bitunary_op>
	create(size_t nbits) const = 0;
};

static inline bool
is_bitunary_op(const jive::operation & op) noexcept
{
	return dynamic_cast<const bitunary_op*>(&op) != nullptr;
}

static inline bool
is_bitunary_node(const jive::node * node) noexcept
{
	return is_opnode<bitunary_op>(node);
}

/* Represents a binary operation (possibly normalized n-ary if associative)
 * on a bitstring of a specific width, produces another bitstring of the
 * same width. */
class bitbinary_op : public jive::binary_op {
public:
	virtual
	~bitbinary_op() noexcept;

	inline
	bitbinary_op(const bittype & type, size_t arity = 2) noexcept
	: binary_op(std::vector<jive::port>(arity, {type}), type)
	{}

	/* reduction methods */
	virtual jive_binop_reduction_path_t
	can_reduce_operand_pair(
		const jive::output * arg1,
		const jive::output * arg2) const noexcept override;

	virtual jive::output *
	reduce_operand_pair(
		jive_binop_reduction_path_t path,
		jive::output * arg1,
		jive::output * arg2) const override;

	virtual bitvalue_repr
	reduce_constants(
		const bitvalue_repr & arg1,
		const bitvalue_repr & arg2) const = 0;

	virtual std::unique_ptr<bitbinary_op>
	create(size_t nbits) const = 0;

	inline const bittype &
	type() const noexcept
	{
		return *static_cast<const bittype*>(&result(0).type());
	}
};

static inline bool
is_bitbinary_op(const jive::operation & op) noexcept
{
	return dynamic_cast<const jive::bitbinary_op*>(&op) != nullptr;
}

static inline bool
is_bitbinary_node(const jive::node * node) noexcept
{
	return is_opnode<bitbinary_op>(node);
}

enum class compare_result {
	undecidable,
	static_true,
	static_false
};

class bitcompare_op : public jive::binary_op {
public:
	virtual
	~bitcompare_op() noexcept;

	inline
	bitcompare_op(const bittype & type) noexcept
	: binary_op({type, type}, bit1)
	{}

	virtual jive_binop_reduction_path_t
	can_reduce_operand_pair(
		const jive::output * arg1,
		const jive::output * arg2) const noexcept override;

	virtual jive::output *
	reduce_operand_pair(
		jive_binop_reduction_path_t path,
		jive::output * arg1,
		jive::output * arg2) const override;

	virtual compare_result
	reduce_constants(
		const bitvalue_repr & arg1,
		const bitvalue_repr & arg2) const = 0;

	virtual std::unique_ptr<bitcompare_op>
	create(size_t nbits) const = 0;

	inline const bittype &
	type() const noexcept
	{
		return *static_cast<const bittype*>(&argument(0).type());
	}
};

static inline bool
is_bitcompare_op(const jive::operation & op) noexcept
{
	return dynamic_cast<const bitcompare_op*>(&op) != nullptr;
}

static inline bool
is_bitcompare_node(const jive::node * node) noexcept
{
	return is_opnode<bitcompare_op>(node);
}

}

#endif
