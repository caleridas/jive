/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2012 2013 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JIVE_TYPES_FLOAT_COMPARISON_FLTLESS_H
#define JIVE_TYPES_FLOAT_COMPARISON_FLTLESS_H

#include <jive/types/float/fltoperation-classes.h>

extern const jive_node_class JIVE_FLTLESS_NODE;

namespace jive {
namespace flt {

bool compute_less(value_repr arg1, value_repr arg2);
extern const char fltless_name[];

typedef detail::make_cmpop<
	compute_less,
	&JIVE_FLTLESS_NODE,
	fltless_name,
	jive_binary_operation_none> less_operation;

}
}

jive::output *
jive_fltless(jive::output * operand1, jive::output * operand2);

#endif
