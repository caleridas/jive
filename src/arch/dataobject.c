/*
 * Copyright 2010 2011 2012 2013 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/arch/dataobject.h>

#include <jive/arch/memlayout.h>
#include <jive/arch/memorytype.h>
#include <jive/types/bitstring/constant.h>
#include <jive/types/bitstring/type.h>
#include <jive/types/record/rcdgroup.h>
#include <jive/types/record/rcdtype.h>
#include <jive/types/union/unntype.h>
#include <jive/types/union/unnunify.h>
#include <jive/vsdg/anchortype.h>
#include <jive/vsdg/controltype.h>
#include <jive/vsdg/graph.h>
#include <jive/vsdg/node-private.h>
#include <jive/vsdg/statetype.h>

jive_node *
jive_dataitems_node_create(jive_region * region, size_t nitems, jive::output * const items[]);

jive_node *
jive_datadef_node_create(jive_region * region, jive::output * data);

jive_node *
jive_dataobj_node_create(jive_region * region, jive::output * anchor);

static inline bool
is_powerof2(size_t v)
{
	return (v & (v-1)) == 0;
}

static void
flatten_data_items(
	jive_context * ctx,
	jive::output * data,
	size_t * ret_nitems,
	jive::output *** ret_items,
	jive_memlayout_mapper * layout_mapper)
{
	size_t nitems = 0;
	jive::output ** items = 0;
	const jive::base::type * type_ = &data->type();
	if (dynamic_cast<const jive::bits::type*>(type_)) {
		const jive::bits::type * type = static_cast<const jive::bits::type*>(type_);
		
		if (type->nbits() < 8 || !is_powerof2(type->nbits()))
			jive_context_fatal_error(
				ctx,
				"Type mismatch: primitive data items must be power-of-two bytes in size");
		
		nitems = type->nbits() / 8;
		items = jive_context_malloc(ctx, sizeof(*items) * nitems);
		
		size_t n;
		for (n = 0; n < nitems; n++)
			items[n] = NULL;
		items[0] = data;
	} else if (dynamic_cast<const jive::rcd::type*>(type_)) {
		const jive::rcd::type * type = static_cast<const jive::rcd::type*>(type_);
		const jive::rcd::declaration * decl = type->declaration();
		const jive_record_memlayout * layout = jive_memlayout_mapper_map_record(layout_mapper, decl);
		
		jive_group_node * node = jive_group_node_cast(data->node());
		if (!node)
			jive_context_fatal_error(ctx, "Type mismatch: can only serialize simple record compounds");
			
		jive_graph * graph = data->node()->graph;
		
		nitems = layout->base.total_size;
		items = jive_context_malloc(ctx, sizeof(*items) * nitems);
		
		jive::output * zero_pad = jive_bitconstant(graph, 8, "00000000");
		size_t n, k;
		for (n = 0; n < nitems; n++)
			items[n] = zero_pad;
			
		for (k = 0; k < decl->nelements; k++) {
			size_t nsub_items;
			jive::output ** sub_items;
			
			flatten_data_items(ctx, data->node()->inputs[k]->origin(), &nsub_items, &sub_items,
				layout_mapper);
			
			if (nsub_items + layout->element[k].offset > nitems)
				jive_context_fatal_error(ctx, "Invalid record layout: element exceeds record");
			
			for (n = 0; n < nsub_items; n++) {
				if (items[n + layout->element[k].offset] != zero_pad)
					jive_context_fatal_error(ctx, "Invalid record layout: members overlap");
				items[n + layout->element[k].offset] = sub_items[n];
			}
			
			jive_context_free(ctx, sub_items);
		}
	} else if (dynamic_cast<const jive::unn::type*>(type_)) {
		const jive::unn::type * type = static_cast<const jive::unn::type*>(type_);
		const jive::unn::declaration * decl = type->declaration();
		const jive_union_memlayout * layout = jive_memlayout_mapper_map_union(layout_mapper, decl);
		
		if (!dynamic_cast<const jive::unn::unify_op *>(&data->node()->operation())) {
			jive_context_fatal_error(ctx, "Type mismatch: can only serialize simple union compounds");
		}
		
		jive_graph * graph = data->node()->graph;
		
		nitems = layout->base.total_size;
		items = jive_context_malloc(ctx, sizeof(*items) * nitems);
		
		jive::output * zero_pad = jive_bitconstant(graph, 8, "00000000");
		size_t n;
		for (n = 0; n < nitems; n++)
			items[n] = zero_pad;
			
		size_t nsub_items;
		jive::output ** sub_items;
		
		flatten_data_items(ctx, data->node()->inputs[0]->origin(), &nsub_items, &sub_items,
			layout_mapper);
		
		if (nsub_items > nitems)
			jive_context_fatal_error(ctx, "Invalid union layout: element exceeds union");
		
		for (n = 0; n < nsub_items; n++) {
			items[n] = sub_items[n];
		}
		
		jive_context_free(ctx, sub_items);
	} else {
		jive_context_fatal_error(
			ctx,
			"Type mismatch: can only serialize record and primitive data items");
	}
	
	*ret_nitems = nitems;
	*ret_items = items;
}

static size_t
squeeze_data_items(size_t nitems, jive::output ** items)
{
	size_t n, k = 0;
	for (n = 0; n < nitems; n++) {
		if (items[n])
			items[k++] = items[n];
	}
	return k;
}

jive::output *
jive_dataobj(jive::output * data, jive_memlayout_mapper * layout_mapper)
{
	jive_graph * graph = data->node()->graph;
	jive_context * context = graph->context;
	jive_region * parent = graph->root_region;
	
	jive_region * region = jive_region_create_subregion(parent);
	region->attrs.section = jive_region_section_data;
	
	size_t ndata_items;
	jive::output ** data_items;
	flatten_data_items(context, data, &ndata_items, &data_items, layout_mapper);
	ndata_items = squeeze_data_items(ndata_items, data_items);
	jive_node * items = jive_dataitems_node_create(region, ndata_items, data_items);
	jive_context_free(context, data_items);
	
	jive_node * datadef = jive_datadef_node_create(region, items->outputs[0]);
	
	jive_node * dataobj = jive_dataobj_node_create(parent, datadef->outputs[0]);
	
	return dataobj->outputs[0];
}

jive::output *
jive_rodataobj(jive::output * data, jive_memlayout_mapper * layout_mapper)
{
	jive_graph * graph = data->node()->graph;
	jive_context * context = graph->context;
	jive_region * parent = graph->root_region;

	jive_region * region = jive_region_create_subregion(parent);
	region->attrs.section = jive_region_section_rodata;

	size_t ndata_items;
	jive::output ** data_items;
	flatten_data_items(context, data, &ndata_items, &data_items, layout_mapper);
	ndata_items = squeeze_data_items(ndata_items, data_items);
	jive_node * items = jive_dataitems_node_create(region, ndata_items, data_items);
	jive_context_free(context, data_items);

	jive_node * datadef = jive_datadef_node_create(region, items->outputs[0]);
	jive_node * dataobj = jive_dataobj_node_create(parent, datadef->outputs[0]);

	return dataobj->outputs[0];
}

jive::output *
jive_bssobj(jive::output * data, jive_memlayout_mapper * layout_mapper)
{
	jive_graph * graph = data->node()->graph;
	jive_context * context = graph->context;
	jive_region * parent = graph->root_region;

	jive_region * region = jive_region_create_subregion(parent);
	region->attrs.section = jive_region_section_bss;

	size_t ndata_items;
	jive::output ** data_items;
	flatten_data_items(context, data, &ndata_items, &data_items, layout_mapper);
	ndata_items = squeeze_data_items(ndata_items, data_items);
	jive_node * items = jive_dataitems_node_create(region, ndata_items, data_items);
	jive_context_free(context, data_items);

	jive_node * datadef = jive_datadef_node_create(region, items->outputs[0]);
	jive_node * dataobj = jive_dataobj_node_create(parent, datadef->outputs[0]);

	return dataobj->outputs[0];
}

static jive_node *
jive_dataitems_node_create_(struct jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, jive::output * const operands[])
{
	return jive_dataitems_node_create(region, noperands, operands);
}

const jive_node_class JIVE_DATAITEMS_NODE = {
	parent : &JIVE_NODE,
	name : "DATAITEMS",
	fini : jive_node_fini_, /* inherit */
	get_default_normal_form : jive_node_get_default_normal_form_, /* inherit */
	get_label : jive_node_get_label_, /* inherit */
	match_attrs : jive_node_match_attrs_, /* inherit */
	check_operands : jive_node_check_operands_, /* inherit */
	create : jive_dataitems_node_create_, /* override */
};

static jive_node *
jive_datadef_node_create_(struct jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, jive::output * const operands[])
{
	return jive_datadef_node_create(region, operands[0]);
}

const jive_node_class JIVE_DATADEF_NODE = {
	parent : &JIVE_NODE,
	name : "DATADEF",
	fini : jive_node_fini_, /* inherit */
	get_default_normal_form : jive_node_get_default_normal_form_, /* inherit */
	get_label : jive_node_get_label_, /* inherit */
	match_attrs : jive_node_match_attrs_, /* inherit */
	check_operands : jive_node_check_operands_, /* inherit */
	create : jive_datadef_node_create_, /* override */
};

static jive_node *
jive_dataobj_node_create_(struct jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, jive::output * const operands[])
{
	return jive_dataobj_node_create(region, operands[0]);
}

const jive_node_class JIVE_DATAOBJ_NODE = {
	parent : &JIVE_NODE,
	name : "DATA",
	fini : jive_node_fini_, /* inherit */
	get_default_normal_form : jive_node_get_default_normal_form_, /* inherit */
	get_label : jive_node_get_label_, /* inherit */
	match_attrs : jive_node_match_attrs_, /* inherit */
	check_operands : jive_node_check_operands_, /* inherit */
	create : jive_dataobj_node_create_, /* override */
};

jive_node *
jive_dataitems_node_create(jive_region * region, size_t nitems, jive::output * const items[])
{
	jive_datadef_node * node = new jive_datadef_node(jive::datadef_operation());
	
	const jive::base::type * item_types[nitems];
	size_t n;
	for (n = 0; n < nitems; n++)
		item_types[n] = &items[n]->type();
	
	node->class_ = &JIVE_DATAITEMS_NODE;
	jive::ctl::type control;
	const jive::base::type * control_ptr = &control;
	jive_node_init_(node, region,
		nitems, item_types, items,
		1, &control_ptr);
	region->bottom = node;
	
	return node;
}

jive_node *
jive_datadef_node_create(jive_region * region, jive::output * data)
{
	jive_datadef_node * node = new jive_datadef_node(jive::datadef_operation());
	
	node->class_ = &JIVE_DATADEF_NODE;
	jive::ctl::type data_type;
	const jive::base::type * data_type_ptr = &data_type;
	jive::achr::type anchor;
	const jive::base::type * ancptr = &anchor;
	jive_node_init_(node, region,
		1, &data_type_ptr, &data,
		1, &ancptr);
	region->bottom = node;
	
	return node;
}

jive_node *
jive_dataobj_node_create(jive_region * region, jive::output * anchor)
{
	jive_dataobj_node * node = new jive_dataobj_node(jive::dataobj_operation());
	
	jive::achr::type anchor_type;
	const jive::base::type * ancptr = &anchor_type;
	/* FIXME: a data object should not have a memory type as output */
	jive::mem::type objstate_type;
	const jive::base::type * typeptr = &objstate_type;
	node->class_ = &JIVE_DATAOBJ_NODE;
	jive_node_init_(node, region,
		1, &ancptr, &anchor,
		1, &typeptr);
	
	return node;
}
