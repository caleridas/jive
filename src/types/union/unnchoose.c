/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/types/union.h>

#include <jive/arch/addresstype.h>
#include <jive/arch/load.h>
#include <jive/types/bitstring/type.h>
#include <jive/util/buffer.h>
#include <jive/vsdg/graph.h>
#include <jive/vsdg/node-private.h>

#include <stdio.h>
#include <string.h>

static constexpr jive_unop_reduction_path_t jive_choose_reduction_load = 128;

namespace jive {
namespace unn {

choose_operation::~choose_operation() noexcept
{
}

bool
choose_operation::operator==(const operation & other) const noexcept
{
	const choose_operation * op =
		dynamic_cast<const choose_operation *>(&other);
	return op && type_ == op->type_ && element_ == op->element_;
}

jive_node *
choose_operation::create_node(
	jive_region * region,
	size_t narguments,
	jive::output * const arguments[]) const
{
	return jive_opnode_create(*this, region, arguments, arguments + narguments);
}

std::string
choose_operation::debug_string() const
{
	char tmp[32];
	snprintf(tmp, sizeof(tmp), "CHOOSE(%zd)", element());
	return tmp;
}

const jive::base::type &
choose_operation::argument_type(size_t index) const noexcept
{
	return type_;
}

const jive::base::type &
choose_operation::result_type(size_t index) const noexcept
{
	return *type_.declaration()->elements[element()];
}

jive_unop_reduction_path_t
choose_operation::can_reduce_operand(
	const jive::output * arg) const noexcept
{
	if (dynamic_cast<const unify_op *>(&arg->node()->operation())) {
		return jive_unop_reduction_inverse;
	}

	if (dynamic_cast<const load_op *>(&arg->node()->operation())) {
		return jive_choose_reduction_load;
	}

	return jive_unop_reduction_none;
}

jive::output *
choose_operation::reduce_operand(
	jive_unop_reduction_path_t path,
	jive::output * arg) const
{
	if (path == jive_unop_reduction_inverse) {
		return arg->node()->inputs[0]->origin();
	}

	if (path == jive_choose_reduction_load) {
		const load_op & op =
			static_cast<const load_op &>(arg->node()->operation());
		jive::output * address = arg->node()->inputs[0]->origin();

		const jive::unn::declaration * decl = static_cast<const jive::unn::output*>(
			arg->node()->outputs[0])->declaration();

		size_t nstates = arg->node()->ninputs-1;
		jive::output * states[nstates];
		for (size_t n = 0; n < nstates; n++) {
			states[n] = arg->node()->inputs[n+1]->origin();
		}
	
		if (dynamic_cast<jive::addr::output*>(address)) {
			return jive_load_by_address_create(address, decl->elements[element()],
				nstates, states);
		} else {
			size_t nbits = static_cast<const jive::bits::output*>(address)->nbits();
			return jive_load_by_bitstring_create(address, nbits, decl->elements[element()],
				nstates, states);
		}
	}

	return nullptr;
}

std::unique_ptr<jive::operation>
choose_operation::copy() const
{
	return std::unique_ptr<jive::operation>(new choose_operation(*this));
}

}
}

jive::output *
jive_choose_create(size_t member, jive::output * argument)
{
	const jive::unn::type & unn_type =
		dynamic_cast<const jive::unn::type &>(argument->type());
	jive::unn::choose_operation op(unn_type, member);
	return jive_node_create_normalized(argument->node()->graph, op, {argument})[0];
}
