#include <jive/arch/call.h>

#include <jive/arch/addresstype.h>
#include <jive/vsdg/node-private.h>
#include <jive/vsdg/region.h>

static void
jive_call_node_fini_(jive_node * self_);

static const jive_node_attrs *
jive_call_node_get_attrs_(const jive_node * self_);

static bool
jive_call_node_match_attrs_(const jive_node * self_, const jive_node_attrs * attrs_);

static jive_node *
jive_call_node_create_(struct jive_region * region, const jive_node_attrs * attrs_,
	size_t noperands, struct jive_output * const operands[]);

const jive_node_class JIVE_CALL_NODE = {
	.parent = &JIVE_NODE,
	.name = "CALL",
	.fini = jive_call_node_fini_, /* override */
	.get_label = jive_node_get_label_, /* inherit */
	.get_attrs = jive_call_node_get_attrs_, /* override */
	.match_attrs = jive_call_node_match_attrs_, /* override */
	.create = jive_call_node_create_, /* override */
	.get_aux_rescls = jive_node_get_aux_rescls_ /* inherit */
};

static void
jive_call_node_fini_(jive_node * self_)
{
	jive_context * context = self_->graph->context;
	jive_call_node * self = (jive_call_node *) self_;
	
	size_t n;
	for (n = 0; n < self->attrs.nreturns; n++) {
		jive_type_fini(self->attrs.return_types[n]);
		jive_context_free(context, self->attrs.return_types[n]);
	}
	
	jive_context_free(context, self->attrs.return_types);
	
	jive_node_fini_(&self->base);
}

static const jive_node_attrs *
jive_call_node_get_attrs_(const jive_node * self_)
{
	const jive_call_node * self = (const jive_call_node *) self_;
	return &self->attrs.base;
}

static bool
jive_call_node_match_attrs_(const jive_node * self_, const jive_node_attrs * attrs_)
{
	const jive_call_node * self = (const jive_call_node *) self_;
	const jive_call_node_attrs * attrs = (const jive_call_node_attrs *) attrs_;
	
	if (self->attrs.nreturns != attrs->nreturns)
		return false;
	
	size_t n;
	for (n = 0; n < attrs->nreturns; n++) {
		if (!jive_type_equals(self->attrs.return_types[n], attrs->return_types[n]))
			return false;
	}
	
	return true;
}

static jive_node *
jive_call_node_create_(jive_region * region, const jive_node_attrs * attrs_,
	size_t noperands, jive_output * const operands[])
{
	JIVE_DEBUG_ASSERT(noperands == 1);
	const jive_call_node_attrs * attrs = (const jive_call_node_attrs *) attrs_;
	return jive_call_node_create(region, operands[0],
		attrs->calling_convention,
		noperands - 1, operands + 1,
		attrs->nreturns, (const jive_type * const *) attrs->return_types);
}

jive_node *
jive_call_node_create(jive_region * region,
	jive_output * target_address, const jive_calling_convention * calling_convention,
	size_t narguments, jive_output * const arguments[],
	size_t nreturns, const jive_type * const return_types[])
{
	jive_context * context = region->graph->context;
	jive_call_node * node = jive_context_malloc(context, sizeof(*node));
	
	size_t n;
	
	node->base.class_ = &JIVE_CALL_NODE;
	
	node->attrs.nreturns = nreturns;
	node->attrs.return_types = jive_context_malloc(context, sizeof(*node->attrs.return_types) * nreturns);
	
	JIVE_DECLARE_ADDRESS_TYPE(address_type);
	jive_output * operands[narguments + 1];
	const jive_type * operand_types[narguments + 1];
	
	operands[0] = target_address;
	operand_types[0] = address_type;
	for (n = 0; n < narguments; n++) {
		operands[n + 1] = arguments[n];
		operand_types[n + 1] = jive_output_get_type(arguments[n]);
	}
	
	for (n = 0; n < nreturns; n++)
		node->attrs.return_types[n] = jive_type_copy(return_types[n], context);
	
	jive_node_init_(&node->base, region,
		narguments + 1, operand_types, operands,
		nreturns, return_types);
	
	return &node->base;
}