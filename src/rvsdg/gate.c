/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/rvsdg/gate.h>
#include <jive/rvsdg/graph.h>
#include <jive/rvsdg/notifiers.h>
#include <jive/rvsdg/resource.h>
#include <jive/rvsdg/type.h>

namespace jive {

gate::gate(
	jive::graph * graph,
	const std::string & name,
	const jive::type & type)
: name_(name)
, graph_(graph)
, type_(type.copy())
, rescls_(&jive_root_resource_class)
{
	graph->gates.push_back(this);
}

gate::gate(
	jive::graph * graph,
	const std::string & name,
	const resource_class * rescls)
: name_(name)
, graph_(graph)
, type_(rescls->type().copy())
, rescls_(rescls)
{
	graph->gates.push_back(this);
}

gate::~gate() noexcept
{
	JIVE_DEBUG_ASSERT(inputs.empty());
	JIVE_DEBUG_ASSERT(outputs.empty());
	JIVE_DEBUG_ASSERT(interference_.empty());

	graph()->gates.erase(this);
}

void
gate::add_interference(jive::gate * other)
{
	JIVE_DEBUG_ASSERT(this != other);

	auto it = interference_.find(other);
	if (it != interference_.end()) {
		it->whole->count++;
		return;
	}

	auto i = new jive_gate_interference;
	i->first.gate = this;
	i->first.whole = i;
	i->second.gate = other;
	i->second.whole = i;
	i->count = 1;

	interference_.insert(&i->second);
	other->interference_.insert(&i->first);

	on_gate_interference_add(this, other);
}

void
gate::clear_interferences()
{
	while (!interference_.empty()) {
		auto i = interference_.begin()->whole;
		i->count = i->count-1;
		if (i->count == 0) {
			i->first.gate->interference_.erase(&i->second);
			i->second.gate->interference_.erase(&i->first);
			on_gate_interference_remove(this, i->second.gate);
			delete i;
		}
	}
}

}
