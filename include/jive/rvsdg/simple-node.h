/*
 * Copyright 2016 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_RVSDG_SIMPLE_NODE_H
#define JIVE_RVSDG_SIMPLE_NODE_H

#include <jive/rvsdg/graph.h>
#include <jive/rvsdg/node.h>
#include <jive/rvsdg/region.h>
#include <jive/rvsdg/simple-normal-form.h>

namespace jive {

class simple_op;
class simple_input;
class simple_output;

/* simple nodes */

class simple_node final : public node {
public:
	virtual
	~simple_node();

private:
	simple_node(
		jive::region * region,
		const jive::simple_op & op,
		const std::vector<jive::output*> & operands);

public:
	jive::simple_input *
	input(size_t index) const noexcept;

	jive::simple_output *
	output(size_t index) const noexcept;

	const jive::simple_op &
	operation() const noexcept;

	virtual jive::node *
	copy(jive::region * region, const std::vector<jive::output*> & operands) const override;

	virtual jive::node *
	copy(jive::region * region, jive::substitution_map & smap) const override;

	static inline jive::simple_node *
	create(
		jive::region * region,
		const jive::simple_op & op,
		const std::vector<jive::output*> & operands)
	{
		return new simple_node(region, op, operands);
	}

	static inline std::vector<jive::output*>
	create_normalized(
		jive::region * region,
		const jive::simple_op & op,
		const std::vector<jive::output*> & operands)
	{
		auto nf = static_cast<simple_normal_form*>(region->graph()->node_normal_form(typeid(op)));
		return nf->normalized_create(region, op, operands);
	}
};

static inline bool
is_simple_node(const jive::node * node) noexcept
{
	return dynamic_cast<const jive::simple_node*>(node) != nullptr;
}

/* inputs */

class simple_input final : public input {
	friend jive::output;

public:
	virtual
	~simple_input() noexcept;

	simple_input(
		jive::simple_node * node,
		size_t index,
		jive::output * origin,
		const jive::port & port);

public:
	virtual jive::simple_node *
	node() const noexcept override;

private:
	jive::simple_node * node_;
};

/* outputs */

class simple_output final : public output {
	friend jive::simple_input;

public:
	virtual
	~simple_output() noexcept;

	simple_output(
		jive::simple_node * node,
		size_t index,
		const jive::port & port);

public:
	virtual jive::simple_node *
	node() const noexcept override;

private:
	jive::simple_node * node_;
};

/* simple node method definitions */

inline jive::simple_input *
simple_node::input(size_t index) const noexcept
{
	return static_cast<simple_input*>(node::input(index));
}

inline jive::simple_output *
simple_node::output(size_t index) const noexcept
{
	return static_cast<simple_output*>(node::output(index));
}

inline const jive::simple_op &
simple_node::operation() const noexcept
{
	return *static_cast<const simple_op*>(&node::operation());
}

}

#endif
