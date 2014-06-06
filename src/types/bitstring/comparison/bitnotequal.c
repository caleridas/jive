/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/types/bitstring/comparison/bitnotequal.h>

#include <jive/types/bitstring/bitoperation-classes-private.h>
#include <jive/types/bitstring/constant.h>
#include <jive/vsdg/control.h>
#include <jive/vsdg/controltype.h>
#include <jive/vsdg/node-private.h>
#include <jive/vsdg/region.h>

static jive_node *
jive_bitnotequal_create_(struct jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, struct jive_output * const operands[]);

static jive_binop_reduction_path_t
jive_bitnotequal_node_can_reduce_operand_pair_(const jive_node_class * cls,
	const jive_node_attrs * attrs, const jive_output * op1, const jive_output * op2);

static jive_output *
jive_bitnotequal_node_reduce_operand_pair_(jive_binop_reduction_path_t path,
	const jive_node_class * cls, const jive_node_attrs * attrs, jive_output * op1, jive_output * op2);

const jive_bitcomparison_operation_class JIVE_BITNOTEQUAL_NODE_ = {
	base : { /* jive_binary_operation_class */
		base : { /* jive_node_class */
			parent : &JIVE_BITCOMPARISON_NODE,
			name : "BITNOTEQUAL",
			fini : jive_node_fini_, /* inherit */
			get_default_normal_form : jive_binary_operation_get_default_normal_form_, /* inherit */
			get_label : jive_node_get_label_, /* inherit */
			match_attrs : jive_node_match_attrs_, /* inherit */
			check_operands : jive_bitcomparison_operation_check_operands_, /* inherit */
			create : jive_bitnotequal_create_, /* override */
		},
		flags : jive_binary_operation_commutative,
		single_apply_under : NULL,
		multi_apply_under : NULL,
		distributive_over : NULL,
		distributive_under : NULL,
	
		can_reduce_operand_pair : jive_bitnotequal_node_can_reduce_operand_pair_, /* override */
		reduce_operand_pair : jive_bitnotequal_node_reduce_operand_pair_ /* override */
	},
	type : jive_bitcmp_code_notequal,
	compare_constants : NULL
};

static void
jive_bitnotequal_node_init_(jive_node * self, jive_region * region,
	jive_output * operand1, jive_output * operand2)
{
	size_t nbits = static_cast<jive::bits::output*>(operand1)->nbits();

	jive::ctl::type ctype;
	const jive::base::type * ctype_ptr = &ctype;
	jive::bits::type btype(nbits);
	const jive::base::type * tmparray0[] = {&btype, &btype};
	jive_output * tmparray1[] = {operand1, operand2};
	jive_node_init_(self, region,
		2, tmparray0, tmparray1,
		1, &ctype_ptr);
}

static jive_node *
jive_bitnotequal_create_(struct jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, struct jive_output * const operands[])
{
	JIVE_DEBUG_ASSERT(noperands == 2);

	jive_node * node = jive::create_operation_node(jive::bitstring::notequal_operation());
	node->class_ = &JIVE_BITNOTEQUAL_NODE;
	jive_bitnotequal_node_init_(node, region, operands[0], operands[1]);

	return node;
}

static jive_binop_reduction_path_t
jive_bitnotequal_node_can_reduce_operand_pair_(const jive_node_class * cls,
	const jive_node_attrs * attrs, const jive_output * op1, const jive_output * op2)
{
	const jive_bitconstant_node * n1 = dynamic_cast<jive_bitconstant_node *>(op1->node());
	const jive_bitconstant_node * n2 = dynamic_cast<jive_bitconstant_node *>(op2->node());

	if (n1 && n2) {
		JIVE_DEBUG_ASSERT(n1->operation().bits.size() == n2->operation().bits.size());
		char result = jive_bitstring_notequal(
			&n1->operation().bits[0], &n2->operation().bits[0], n1->operation().bits.size());

		switch (result) {
			case '0': return 1;
			case '1': return 2;
			default: return jive_binop_reduction_none;
		}
	}

	return jive_binop_reduction_none;
}

static jive_output *
jive_bitnotequal_node_reduce_operand_pair_(
	jive_binop_reduction_path_t path, const jive_node_class * cls,
	const jive_node_attrs * attrs, jive_output * op1, jive_output * op2)
{
	jive_graph * graph = op1->node()->graph;

	switch (path) {
		case 1:
			return jive_control_false(graph);
		case 2:
			return jive_control_true(graph);
		default:
			return NULL;
	}
}

jive_output *
jive_bitnotequal(jive_output * operand1, jive_output * operand2)
{
	jive_graph * graph = operand1->node()->graph;
	jive_output * tmparray2[] = {operand1, operand2};
	jive::bitstring::notequal_operation op;
	return jive_binary_operation_create_normalized(&JIVE_BITNOTEQUAL_NODE_.base, graph, &op, 2,
		tmparray2);
}
