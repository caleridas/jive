#include <jive/arch/load.h>

#include <jive/vsdg/valuetype.h>
#include <jive/vsdg/node-private.h>
#include <jive/types/bitstring/type.h>
#include <jive/arch/addresstype.h>

static void
jive_load_node_fini_(jive_node * self_);

static const jive_node_attrs *
jive_load_node_get_attrs_(const jive_node * self_);

static bool
jive_load_node_match_attrs_(const jive_node * self_, const jive_node_attrs * attrs_);

static jive_node *
jive_load_node_create_(struct jive_region * region, const jive_node_attrs * attrs_,
	size_t noperands, struct jive_output * const operands[]);

const jive_node_class JIVE_LOAD_NODE = {
	.parent = &JIVE_NODE,
	.name = "LOAD",
	.fini = jive_load_node_fini_, /* override */
	.get_default_normal_form = jive_node_get_default_normal_form_, /* inherit */
	.get_label = jive_node_get_label_, /* inherit */
	.get_attrs = jive_load_node_get_attrs_, /* override */
	.match_attrs = jive_load_node_match_attrs_, /* override */
	.create = jive_load_node_create_, /* override */
	.get_aux_rescls = jive_node_get_aux_rescls_ /* inherit */
};

static void
jive_load_node_fini_(jive_node * self_)
{
	jive_context * context = self_->graph->context;
	jive_load_node * self = (jive_load_node *) self_;
	
	jive_type_fini(&self->attrs.datatype->base);
	jive_context_free(context, self->attrs.datatype);
	
	jive_node_fini_(&self->base);
}

static const jive_node_attrs *
jive_load_node_get_attrs_(const jive_node * self_)
{
	const jive_load_node * self = (const jive_load_node *) self_;
	return &self->attrs.base;
}

static bool
jive_load_node_match_attrs_(const jive_node * self_, const jive_node_attrs * attrs_)
{
	const jive_load_node * self = (const jive_load_node *) self_;
	const jive_load_node_attrs * attrs = (const jive_load_node_attrs *) attrs_;
	return jive_type_equals(&self->attrs.datatype->base, &attrs->datatype->base);
}

static jive_node *
jive_load_node_create_(jive_region * region, const jive_node_attrs * attrs_,
	size_t noperands, jive_output * const operands[])
{
	const jive_load_node_attrs * attrs = (const jive_load_node_attrs *) attrs_;

	if(jive_output_isinstance(operands[0], &JIVE_BITSTRING_OUTPUT)){
		size_t nbits = jive_bitstring_output_nbits((const jive_bitstring_output *) operands[0]);
		return jive_load_by_bitstring_node_create(region, operands[0], nbits, attrs->datatype,
			noperands-1, &operands[1]);
	} else {
		return jive_load_by_address_node_create(region, operands[0], attrs->datatype,
			noperands-1, &operands[1]);
	}
}

void
jive_load_node_init_(jive_load_node * self, jive_region * region,
	jive_output * address, const jive_type * address_type,
	const jive_value_type * datatype,
	size_t nstates, jive_output * const states[])
{
	jive_context * context = region->graph->context;

	jive_node_init_(&self->base, region,
		1, &address_type, &address,
		1, (const jive_type * []){&datatype->base});
	self->attrs.datatype = (jive_value_type *) jive_type_copy(&datatype->base, context);
	
	/* FIXME: check the type of the states */
	size_t n;
	for (n = 0; n < nstates; n++) {
		const jive_type * type = jive_output_get_type(states[n]);
		jive_node_add_input(&self->base, type, states[n]);
	}
}

static inline jive_region *
load_node_region_innermost(jive_output * address, size_t nstates, jive_output * const states[])
{
	size_t i;
	jive_output * outputs[nstates+1];
	for(i = 0; i < nstates; i++){
		outputs[i] = states[i];
	}
	outputs[nstates] = address;
	
	return jive_region_innermost(nstates+1, outputs);
}

jive_node *
jive_load_by_address_node_create(jive_region * region,
	jive_output * address,
	const jive_value_type * datatype,
	size_t nstates, jive_output * const states[])
{
	jive_load_node * node = jive_context_malloc(region->graph->context, sizeof(*node));
	
	node->base.class_ = &JIVE_LOAD_NODE;
	JIVE_DECLARE_ADDRESS_TYPE(address_type);
	jive_load_node_init_(node, region, address, address_type, datatype, nstates, states);
	
	return &node->base;
}

jive_output *
jive_load_by_address_create(jive_output * address,
	const jive_value_type * datatype,
	size_t nstates, jive_output * const states[])
{
	jive_region * region = load_node_region_innermost(address, nstates, states);
	jive_node * node = jive_load_by_address_node_create(region, address, datatype,
		nstates, states);

	return node->outputs[0];	
}

jive_node *
jive_load_by_bitstring_node_create(jive_region * region,
	jive_output * address, size_t nbits,
	const jive_value_type * datatype,
	size_t nstates, jive_output * const states[])
{
	jive_load_node * node = jive_context_malloc(region->graph->context, sizeof(*node));

	node->base.class_ = &JIVE_LOAD_NODE;
	JIVE_DECLARE_BITSTRING_TYPE(address_type, nbits);
	jive_load_node_init_(node, region, address, address_type, datatype, nstates, states);

	return &node->base;
}

jive_output *
jive_load_by_bitstring_create(jive_output * address, size_t nbits,
	const jive_value_type * datatype,
	size_t nstates, jive_output * const states[])
{
	jive_region * region = load_node_region_innermost(address, nstates, states);
	jive_node * node = jive_load_by_bitstring_node_create(region, address, nbits,
		datatype, nstates, states);

	return node->outputs[0];
}
