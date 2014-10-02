/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * See COPYING for terms of redistribution.
 */

#include <jive/vsdg/node-normal-form.h>

#include <cxxabi.h>
#include <typeindex>
#include <unordered_map>

#include <jive/vsdg/graph.h>

namespace jive {

node_normal_form::~node_normal_form() noexcept
{
}

bool
node_normal_form::normalize_node(jive_node * node) const
{
	return true;
}

bool
node_normal_form::operands_are_normalized(
	const jive::operation & op,
	const std::vector<jive::output *> & arguments) const
{
	return true;
}

std::vector<jive::output *>
node_normal_form::normalized_create(
	const jive::operation & op,
	const std::vector<jive::output *> & arguments) const
{
	jive_region * region = graph()->root_region;
	if (!arguments.empty()) {
		region = jive_region_innermost(arguments.size(), &arguments[0]);
	}

	jive_node * node = nullptr;
	if (get_mutable() && get_cse()) {
		node = jive_node_cse(region, op, arguments);
	}

	if (!node) {
		node = op.create_node(region, arguments.size(), &arguments[0]);
	}

	return std::vector<jive::output *>(&node->outputs[0], &node->outputs[node->noutputs]);
}

void
node_normal_form::set_mutable(bool enable)
{
	if (enable_mutable_ == enable) {
		return;
	}

	children_set<node_normal_form, &node_normal_form::set_mutable>(enable);

	enable_mutable_ = enable;
	if (enable) {
		jive_graph_mark_denormalized(graph());
	}
}

void
node_normal_form::set_cse(bool enable)
{
	if (enable_cse_ == enable) {
		return;
	}

	children_set<node_normal_form, &node_normal_form::set_cse>(enable);

	enable_cse_ = enable;
	if (enable && enable_mutable_) {
		jive_graph_mark_denormalized(graph());
	}
}

namespace {

typedef jive::node_normal_form *(*create_node_normal_form_functor)(
	const std::type_info & operator_class,
	jive::node_normal_form * parent,
	jive_graph * graph);

typedef std::unordered_map<std::type_index, create_node_normal_form_functor>
	node_normal_form_registry;

std::unique_ptr<node_normal_form_registry> registry;

create_node_normal_form_functor
lookup_factory_functor(const std::type_info * info)
{
	if (!registry) {
		registry.reset(new node_normal_form_registry());
	}

	node_normal_form_registry::const_iterator i;
	for(;;) {
		auto i = registry->find(std::type_index(*info));
		if (i != registry->end()) {
			return i->second;
		}
		const auto& cinfo = dynamic_cast<const abi::__si_class_type_info &>(
			*info);
		info = cinfo.__base_type;
	}
}

}

void
node_normal_form::register_factory(
	const std::type_info & operator_class,
	jive::node_normal_form *(*fn)(
		const std::type_info & operator_class,
		jive::node_normal_form * parent,
		jive_graph * graph))
{
	if (!registry) {
		registry.reset(new node_normal_form_registry());
	}

	(*registry)[std::type_index(operator_class)] = fn;
}

node_normal_form *
node_normal_form::create(
	const std::type_info & operator_class,
	jive::node_normal_form * parent,
	jive_graph * graph)
{
	return lookup_factory_functor(&operator_class)(operator_class, parent, graph);
}

}