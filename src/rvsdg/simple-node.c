/*
 * Copyright 2016 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/rvsdg/graph.h>
#include <jive/rvsdg/notifiers.h>
#include <jive/rvsdg/region.h>
#include <jive/rvsdg/simple-node.h>
#include <jive/rvsdg/simple-normal-form.h>
#include <jive/rvsdg/substitution.h>

#include <numeric>

namespace jive {

/* inputs */

simple_input::~simple_input() noexcept
{
	on_input_destroy(this);

	if (port().gate())
		port().gate()->clear_interferences();
}

simple_input::simple_input(
	jive::simple_node * node,
	size_t index,
	jive::output * origin,
	const jive::port & port)
	: input(index, origin, node->region(), port)
	, node_(node)
{
	if (port.gate()) {
		for (size_t n = 0; n < index; n++) {
			auto other = dynamic_cast<jive::simple_input*>(node->input(n));
			if (!other->port().gate()) continue;
			port.gate()->add_interference(other->port().gate());
		}
	}
}

jive::simple_node *
simple_input::node() const noexcept
{
	return node_;
}

/* outputs */

simple_output::simple_output(
	jive::simple_node * node,
	size_t index,
	const jive::port & port)
: output(index, node->region(), port)
, node_(node)
{
	if (port.gate()) {
		for (size_t n = 0; n < index; n++) {
			auto other = node->output(n);
			if (!other->port().gate()) continue;
			port.gate()->add_interference(other->port().gate());
		}
	}
}

simple_output::~simple_output() noexcept
{
	on_output_destroy(this);

	if (port().gate())
		port().gate()->clear_interferences();
}

jive::simple_node *
simple_output::node() const noexcept
{
	return node_;
}

/* simple nodes */

simple_node::~simple_node()
{
	on_node_destroy(this);
}

simple_node::simple_node(
	jive::region * region,
	const jive::simple_op & op,
	const std::vector<jive::output*> & operands)
	: node(op.copy(), region)
{
	if (operation().narguments() != operands.size())
		throw jive::compiler_error(jive::detail::strfmt("Argument error - expected ",
			operation().narguments(), ", received ", operands.size(), " arguments."));

	for (size_t n = 0; n < operation().narguments(); n++) {
		node::add_input(std::move(std::unique_ptr<jive::input>(
			new simple_input(this, n, operands[n], operation().argument(n)))));
	}

	for (size_t n = 0; n < operation().nresults(); n++)
		node::add_output(std::move(std::unique_ptr<jive::output>(
			new simple_output(this, n, operation().result(n)))));

	on_node_create(this);
}

jive::node *
simple_node::copy(jive::region * region, const std::vector<jive::output*> & operands) const
{
	auto node = create(region, *static_cast<const simple_op*>(&operation()), operands);
	graph()->mark_denormalized();
	return node;
}

jive::node *
simple_node::copy(jive::region * region, jive::substitution_map & smap) const
{
	std::vector<jive::output*> operands;
	for (size_t n = 0; n < ninputs(); n++) {
		auto operand = smap.lookup(input(n)->origin());
		operands.push_back(operand ? operand : input(n)->origin());
	}

	auto node = copy(region, operands);

	JIVE_DEBUG_ASSERT(node->noutputs() == noutputs());
	for (size_t n = 0; n < node->noutputs(); n++)
		smap.insert(output(n), node->output(n));

	return node;
}

}
