#ifndef JIVE_TYPES_BITSTRING_COMPARISON_BITEQUAL_H
#define JIVE_TYPES_BITSTRING_COMPARISON_BITEQUAL_H

#include <jive/types/bitstring/bitoperation-classes.h>

extern const jive_bitcomparison_operation_class JIVE_BITEQUAL_NODE_;
#define JIVE_BITEQUAL_NODE (JIVE_BITEQUAL_NODE_.base.base)

jive_node *
jive_bitequal_create(struct jive_region * region,
	struct jive_output * operand1, struct jive_output * operand2);

jive_output *
jive_bitequal(struct jive_output * operand1, struct jive_output * operand2);

JIVE_EXPORTED_INLINE jive_node *
jive_bitequal_node_cast(jive_node * node)
{
	if (jive_node_isinstance(node, &JIVE_BITEQUAL_NODE))
		return node;
	else
		return NULL;
}

#endif
