/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 2016 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_TYPES_FUNCTION_FCTLAMBDA_H
#define JIVE_TYPES_FUNCTION_FCTLAMBDA_H

#include <memory>
#include <vector>

#include <jive/types/function/fcttype.h>
#include <jive/vsdg/operators/structural.h>
#include <jive/vsdg/simple_node.h>

namespace jive {
namespace fct {

class lambda_op final : public structural_op {
public:
	virtual
	~lambda_op() noexcept;

	inline
	lambda_op(const lambda_op & other) = default;

	inline
	lambda_op(lambda_op && other) = default;

	inline lambda_op(
		jive::fct::type function_type,
		std::vector<std::string> argument_names,
		std::vector<std::string> result_names) noexcept
		: function_type_(std::move(function_type))
		, argument_names_(std::move(argument_names))
		, result_names_(std::move(result_names))
	{
	}

	virtual bool
	operator==(const operation & other) const noexcept override;

	virtual size_t
	narguments() const noexcept override;

	virtual size_t
	nresults() const noexcept override;

	virtual const jive::base::type &
	result_type(size_t index) const noexcept override;
	virtual std::string
	debug_string() const override;

	inline const jive::fct::type &
	function_type() const noexcept
	{
		return function_type_;
	}

	inline const std::vector<std::string> &
	argument_names() const noexcept
	{
		return argument_names_;
	}

	inline const std::vector<std::string> &
	result_names() const noexcept
	{
		return result_names_;
	}

	virtual std::unique_ptr<jive::operation>
	copy() const override;

private:
	jive::fct::type function_type_;
	std::vector<std::string> argument_names_;
	std::vector<std::string> result_names_;
};

}
}

bool
jive_lambda_is_self_recursive(const jive::node * self);

void
jive_inline_lambda_apply(jive::node * apply_node);

/* lambda instantiation */

/**
	\brief Represent a lambda construct under construction
*/

namespace jive {

class structural_input;
class structural_node;

namespace fct {

struct lambda_dep {
	jive::structural_input * input;
	jive::oport * output;
};

}
}

typedef struct jive_lambda jive_lambda;

struct jive_lambda {
	jive::structural_node * node;
	struct jive::region * region;
	size_t narguments;
	jive::oport ** arguments;
	std::vector<jive::fct::lambda_dep> depvars;
};

namespace jive {
namespace fct {

lambda_dep
lambda_dep_add(jive_lambda * self, jive::oport * value);

}
}

/**
	\brief Begin constructing a lambda region
*/
struct jive_lambda *
jive_lambda_begin(jive::region * parent,
	const std::vector<std::pair<const jive::base::type*, std::string>> & arguments,
	const std::vector<std::pair<const jive::base::type*, std::string>> & results);

/**
	\brief End constructing a lambda region
*/
jive::oport *
jive_lambda_end(struct jive_lambda * lambda, size_t nresults,
	const jive::base::type * const result_types[], struct jive::oport * const results[]);

#endif
