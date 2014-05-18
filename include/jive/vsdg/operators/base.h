/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_VSDG_OPERATORS_BASE_H
#define JIVE_VSDG_OPERATORS_BASE_H

#include <string>

class jive_input;
class jive_node;
class jive_output;
class jive_region;
class jive_type;
class jive_node_normal_form;

namespace jive {

class operation {
public:
	virtual ~operation() noexcept;

	virtual bool
	operator==(const operation & other) const noexcept;

	virtual size_t
	narguments() const noexcept;

	virtual const jive_type &
	argument_type(size_t index) const noexcept;

	virtual size_t
	nresults() const noexcept;

	virtual const jive_type &
	result_type(size_t index) const noexcept;

	virtual jive_node *
	create_node(
		jive_region * region,
		size_t narguments,
		jive_output * const arguments[]) const;

	virtual std::string
	debug_string() const;

};

}

#endif
