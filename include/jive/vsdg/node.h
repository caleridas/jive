/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 2015 2016 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_VSDG_NODE_H
#define JIVE_VSDG_NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unordered_set>
#include <utility>

#include <jive/util/intrusive-list.h>
#include <jive/util/strfmt.h>
#include <jive/vsdg/basetype.h>
#include <jive/vsdg/gate-interference.h>
#include <jive/vsdg/operation.h>
#include <jive/vsdg/resource.h>
#include <jive/vsdg/tracker.h>

namespace jive {
namespace base {
	class type;
}

class gate;
class graph;
class node_normal_form;
class output;
class substitution_map;

/* inputs */

class input {
public:
	virtual
	~input() noexcept;

	input(
		size_t index,
		jive::output * origin,
		jive::region * region,
		const jive::base::type & type);

	input(
		size_t index,
		jive::output * origin,
		jive::region * region,
		jive::gate * gate);

	input(
		size_t index,
		jive::output * origin,
		jive::region * region,
		const struct jive_resource_class * rescls);

	input(const input &) = delete;

	input(const input &&) = delete;

	input &
	operator=(const input &) = delete;

	input &
	operator=(const input &&) = delete;

	inline size_t
	index() const noexcept
	{
		return index_;
	}

	jive::output *
	origin() const noexcept
	{
		return origin_;
	}

	inline jive::gate *
	gate() const noexcept
	{
		return gate_;
	}

	inline const struct jive_resource_class *
	rescls() const noexcept
	{
		return rescls_;
	}

	void
	divert_origin(jive::output * new_origin);

	inline const jive::base::type &
	type() const noexcept
	{
		return *type_;
	}

	inline jive::region *
	region() const noexcept
	{
		return region_;
	}

	virtual jive::node *
	node() const noexcept = 0;

	virtual std::string
	debug_string() const;

	struct {
		jive::input * prev;
		jive::input * next;
	} gate_input_list;

protected:
	inline void
	set_index(size_t index) noexcept
	{
		index_ = index;
	}

private:
	size_t index_;
	jive::gate * gate_;
	jive::output * origin_;
	jive::region * region_;
	std::unique_ptr<jive::base::type> type_;
	const struct jive_resource_class * rescls_;
};

/* outputs */

class output {
	friend input;

	typedef std::unordered_set<jive::input*>::const_iterator user_iterator;

public:
	virtual
	~output() noexcept;

	output(
		size_t index,
		jive::region * region,
		const jive::base::type & type);

	output(
		size_t index,
		jive::region * region,
		jive::gate * gate);

	/*
		FIXME: The type parameter is currently necessary, since the jive_root_resource_class has no
		       type.
	*/
	output(
		size_t index,
		jive::region * region,
		const jive::base::type & type,
		const struct jive_resource_class * rescls);

	output(const output &) = delete;

	output(const output &&) = delete;

	output &
	operator=(const output &) = delete;

	output &
	operator=(const output &&) = delete;

	inline size_t
	index() const noexcept
	{
		return index_;
	}

	inline size_t
	nusers() const noexcept
	{
		return users_.size();
	}

	inline void
	replace(jive::output * new_origin)
	{
		while (users_.size())
			(*users_.begin())->divert_origin(new_origin);
	}

	inline jive::gate *
	gate() const noexcept
	{
		return gate_;
	}

	inline const struct jive_resource_class *
	rescls() const noexcept
	{
		return rescls_;
	}

	inline user_iterator
	begin() const noexcept
	{
		return users_.begin();
	}

	inline user_iterator
	end() const noexcept
	{
		return users_.end();
	}

	inline const jive::base::type &
	type() const noexcept
	{
		return *type_;
	}

	inline jive::region *
	region() const noexcept
	{
		return region_;
	}

	virtual jive::node *
	node() const noexcept = 0;

	virtual std::string
	debug_string() const;

	struct {
		jive::output * prev;
		jive::output * next;
	} gate_output_list;

protected:
	inline void
	set_index(size_t index) noexcept
	{
		index_ = index;
	}

private:
	void
	remove_user(jive::input * user);

	void
	add_user(jive::input * user);

	size_t index_;
	jive::gate * gate_;
	jive::region * region_;
	std::unordered_set<jive::input*> users_;
	std::unique_ptr<jive::base::type> type_;
	const struct jive_resource_class * rescls_;
};

/**	@}	*/

/**
        \defgroup jive::gate Gates
        Gates
        @{
*/

class gate final {
public:
	~gate() noexcept;

	gate(
		jive::graph * graph,
		const std::string & name,
		const jive::base::type & type);

	gate(
		jive::graph * graph,
		const std::string & name,
		const struct jive_resource_class * rescls);

	const jive::base::type &
	type() const noexcept
	{
		return *type_;
	}

	inline std::string
	debug_string() const
	{
		return name();
	}

	inline jive::graph *
	graph() const noexcept
	{
		return graph_;
	}

	inline const std::string &
	name() const noexcept
	{
		return name_;
	}

	inline const struct jive_resource_class *
	rescls() const noexcept
	{
		return rescls_;
	}

	struct {
		jive::gate * prev;
		jive::gate * next;
	} graph_gate_list;

	struct {
		jive::input * first;
		jive::input * last;
	} inputs;

	struct {
		jive::output * first;
		jive::output * last;
	}	outputs;

	bool may_spill;
	jive_gate_interference_hash interference;

private:
	std::string name_;
	jive::graph * graph_;
	const struct jive_resource_class * rescls_;

	/*
		FIXME: This attribute is necessary as long as the number of inputs do not coincide with the
		number given by the operation. Once this is fixed, the attribute can be removed and the type
		can be taken from the operation.
	*/
	std::unique_ptr<jive::base::type> type_;
};

}	//jive namespace

/**	@}	*/

/**	@}	*/

typedef struct jive_tracker_nodestate jive_tracker_nodestate;

struct jive_resource_class_count;
struct jive_substitution_map;

namespace jive {

class node {
public:
	virtual
	~node();

	node(std::unique_ptr<jive::operation> op, jive::region * region, size_t depth);

	inline const jive::operation &
	operation() const noexcept
	{
		return *operation_;
	}

	virtual bool
	has_users() const noexcept = 0;

	virtual bool
	has_successors() const noexcept = 0;

	inline size_t
	noperands() const noexcept
	{
		return operation_->narguments();
	}

	virtual jive::input *
	add_input(const jive::base::type * type, jive::output * origin) = 0;

	virtual jive::input *
	add_input(jive::gate * gate, jive::output * origin) = 0;

	virtual jive::input *
	add_input(const struct jive_resource_class * rescls, jive::output * origin) = 0;

	virtual void
	remove_input(size_t index) = 0;

	virtual jive::output *
	add_output(const jive::base::type * type) = 0;

	virtual jive::output *
	add_output(const struct jive_resource_class * rescls) = 0;

	virtual jive::output *
	add_output(jive::gate * gate) = 0;

	virtual void
	remove_output(size_t index) = 0;

	inline jive::graph *
	graph() const noexcept
	{
		return graph_;
	}

	inline jive::region *
	region() const noexcept
	{
		return region_;
	}

	virtual size_t
	ninputs() const noexcept = 0;

	virtual size_t
	noutputs() const noexcept = 0;

	virtual jive::node *
	copy(jive::region * region, const std::vector<jive::output*> & operands) const = 0;

	/**
		\brief Copy a node with substitutions
		\param self Node to be copied
		\param target Target region to create node in
		\param substitution Operand and gate substitutions
		\return Copied node

		Create a new node that is semantically equivalent to an
		existing node. The newly created node will use the same
		operands as the existing node unless there is a substitution
		registered for a particular operand.

		The given substitution map is updated so that all
		outputs of the original node will be substituted by
		corresponding outputs of the newly created node in
		subsequent \ref copy operations.
	*/
	virtual jive::node *
	copy(jive::region * region, jive::substitution_map & smap) const = 0;

	virtual jive::input *
	input(size_t index) const noexcept = 0;

	virtual jive::output *
	output(size_t index) const noexcept = 0;

	inline size_t
	depth() const noexcept
	{
		return depth_;
	}

	/*
		FIXME: privatize or completely remove it again
	*/
	void
	recompute_depth();

	struct {
		jive::node * prev;
		jive::node * next;
	} region_top_node_list;
	
	struct {
		jive::node * prev;
		jive::node * next;
	} region_bottom_list;

	std::vector<jive_tracker_nodestate*> tracker_slots;

private:
	jive::detail::intrusive_list_anchor<jive::node> region_node_list_anchor_;

public:
	typedef jive::detail::intrusive_list_accessor<
		jive::node,
		&jive::node::region_node_list_anchor_
	> region_node_list_accessor;

private:
	size_t depth_;
	jive::graph * graph_;
	jive::region * region_;
	std::unique_ptr<jive::operation> operation_;
};

}

struct jive_tracker_nodestate {
	jive::node * node;
	size_t cookie;
	size_t state;
	intptr_t tag;
	struct {
		jive_tracker_nodestate * prev;
		jive_tracker_nodestate * next;
	} state_node_list;
};

static inline jive::input *
jive_node_get_gate_input(const jive::node * self, const jive::gate * gate)
{
	for (size_t n = 0; n < self->ninputs(); n++) {
		if (self->input(n)->gate() == gate) {
			return self->input(n);
		}
	}
	return nullptr;
}

static inline jive::input *
jive_node_get_gate_input(const jive::node * self, const char * name)
{
	for (size_t n = 0; n < self->ninputs(); n++) {
		auto i = self->input(n);
		if (i->gate() && i->gate()->name() == name)
			return i;
	}
	return nullptr;
}

static inline jive::output *
jive_node_get_gate_output(const jive::node * self, const jive::gate * gate)
{
	for (size_t n = 0; n < self->noutputs(); n++) {
		if (self->output(n)->gate() == gate) {
			return self->output(n);
		}
	}
	return nullptr;
}

static inline jive::output *
jive_node_get_gate_output(const jive::node * self, const char * name)
{
	for (size_t n = 0; n < self->noutputs(); n++) {
		auto o = self->output(n);
		if (o->gate() && o->gate()->name() == name)
			return o;
	}
	return nullptr;
}

void
jive_node_get_use_count_input(
	const jive::node * self,
	struct jive_resource_class_count * use_count);

void
jive_node_get_use_count_output(
	const jive::node * self,
	struct jive_resource_class_count * use_count);

static inline std::vector<jive::output*>
jive_node_arguments(const jive::node * self)
{
	std::vector<jive::output*> arguments;
	for (size_t n = 0; n < self->noperands(); ++n) {
		arguments.push_back(self->input(n)->origin());
	}
	return arguments;
}

jive::node *
jive_node_cse(
	jive::region * region,
	const jive::operation & op,
	const std::vector<jive::output*> & arguments);

/* normal forms */

bool
jive_node_normalize(jive::node * self);

/* tracking support */

jive_tracker_nodestate *
jive_node_get_tracker_state_slow(jive::node * self, jive_tracker_slot slot);

static inline jive_tracker_nodestate *
jive_node_get_tracker_state(jive::node * self, jive_tracker_slot slot)
{
	jive_tracker_nodestate * nodestate;
	if (slot.index < self->tracker_slots.size()) {
		nodestate = self->tracker_slots[slot.index];
		if (nodestate->cookie != slot.cookie) {
			nodestate->state = jive_tracker_nodestate_none;
			nodestate->cookie = slot.cookie;
			nodestate->tag = 0;
		}
		return nodestate;
	}
	return jive_node_get_tracker_state_slow(self, slot);
}

#endif
