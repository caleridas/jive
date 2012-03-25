#include <jive/vsdg/gamma.h>

#include <string.h>
#include <stdio.h>

#include <jive/common.h>
#include <jive/context.h>
#include <jive/vsdg/anchortype.h>
#include <jive/vsdg/control.h>
#include <jive/vsdg/graph.h>
#include <jive/vsdg/node-private.h>
#include <jive/vsdg/region.h>
#include <jive/vsdg/traverser.h>

static jive_node *
jive_gamma_tail_node_create_(jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, jive_output * const operands[])
{
	JIVE_DEBUG_ASSERT(noperands == 0);
	
	jive_node * self = jive_context_malloc(region->graph->context, sizeof(*self));
	JIVE_DECLARE_ANCHOR_TYPE(anchor);
	self->class_ = &JIVE_GAMMA_TAIL_NODE;
	jive_node_init_(self, region,
		0, NULL, NULL,
		1, &anchor);
	
	region->bottom = self;
	return self;
}

static jive_node *
jive_gamma_node_create_(jive_region * region, const jive_node_attrs * attrs,
	size_t noperands, jive_output * const operands[])
{
	JIVE_DEBUG_ASSERT(noperands == 3);
	jive_node * self = jive_context_malloc(region->graph->context, sizeof(*self));;
	JIVE_DECLARE_ANCHOR_TYPE(anchor);
	JIVE_DECLARE_CONTROL_TYPE(ctl);
	self->class_ = &JIVE_GAMMA_NODE;
	jive_node_init_(self, region,
		3, (const jive_type *[]){anchor, anchor, ctl}, operands,
		0, NULL);
	
	return self;
}

jive_node_normal_form *
jive_gamma_node_get_default_normal_form_(const jive_node_class * cls, jive_node_normal_form * parent, jive_graph * graph)
{
	jive_gamma_normal_form * normal_form;
	normal_form = jive_context_malloc(graph->context, sizeof(*normal_form));
	normal_form->base.class_ = &JIVE_GAMMA_NORMAL_FORM;
	jive_gamma_normal_form_init_(normal_form, cls, parent, graph);
	return &normal_form->base;
}


const jive_node_class JIVE_GAMMA_TAIL_NODE = {
	.parent = &JIVE_NODE,
	.name = "GAMMA_TAIL",
	.fini = jive_node_fini_,  /* inherit */
	.get_default_normal_form = jive_node_get_default_normal_form_,  /* inherit */
	.get_label = jive_node_get_label_,  /* inherit */
	.get_attrs = jive_node_get_attrs_,  /* inherit */
	.match_attrs = jive_node_match_attrs_,  /* inherit */
	.create = jive_gamma_tail_node_create_,  /* override */
	.get_aux_rescls = jive_node_get_aux_rescls_  /* inherit */
};

const jive_node_class JIVE_GAMMA_NODE = {
	.parent = &JIVE_NODE,
	.name = "GAMMA",
	.fini = jive_node_fini_,  /* inherit */
	.get_default_normal_form = jive_gamma_node_get_default_normal_form_,  /* override */
	.get_label = jive_node_get_label_,  /* inherit */
	.get_attrs = jive_node_get_attrs_,  /* inherit */
	.match_attrs = jive_node_match_attrs_,  /* inherit */
	.create = jive_gamma_node_create_,  /* override */
	.get_aux_rescls = jive_node_get_aux_rescls_  /* inherit */
};

static jive_node *
jive_gamma_tail_node_create(jive_region * region)
{
	jive_node * self = jive_context_malloc(region->graph->context, sizeof(*self));
	JIVE_DECLARE_ANCHOR_TYPE(anchor);
	self->class_ = &JIVE_GAMMA_TAIL_NODE;
	jive_node_init_(self, region,
		0, NULL, NULL,
		1, &anchor);
	
	region->bottom = self;
	return self;
}

static jive_node *
jive_gamma_node_create(jive_region * region,
	jive_output * predicate,
	jive_output * true_alternative,
	jive_output * false_alternative)
{
	jive_node * self = jive_context_malloc(region->graph->context, sizeof(*self));;
	JIVE_DECLARE_ANCHOR_TYPE(anchor);
	JIVE_DECLARE_CONTROL_TYPE(ctl);
	self->class_ = &JIVE_GAMMA_NODE;
	jive_node_init_(self, region,
		3, (const jive_type *[]){anchor, anchor, ctl}, (jive_output *[]){true_alternative, false_alternative, predicate},
		0, NULL);
	
	return self;
}

static jive_node *
jive_gamma_create(
	jive_region * region,
	jive_output * predicate,
	size_t nvalues, const jive_type * const types[],
	jive_output * const true_values[],
	jive_output * const false_values[])
{
	jive_region * false_region = jive_region_create_subregion(region);
	jive_region * true_region = jive_region_create_subregion(region);
	jive_node * false_alt = jive_gamma_tail_node_create(false_region);
	jive_node * true_alt = jive_gamma_tail_node_create(true_region);
	jive_node * gamma = jive_gamma_node_create(region, predicate, false_alt->outputs[0], true_alt->outputs[0]);
	
	size_t n;
	for (n = 0; n < nvalues; n++) {
		char name[80];
		snprintf(name, sizeof(name), "gamma_%p_%zd", gamma, n);
		jive_gate * gate = jive_type_create_gate(types[n], region->graph, name);
		jive_node_gate_input(false_alt, gate, true_values[n]);
		jive_node_gate_input(true_alt, gate, false_values[n]);
		jive_node_gate_output(gamma, gate);
	}
	return gamma;
}

void
jive_gamma(struct jive_output * predicate,
	size_t nvalues, const struct jive_type * const types[],
	struct jive_output * const true_values[],
	struct jive_output * const false_values[],
	struct jive_output * results[])
{
	size_t n;
	
	jive_graph * graph = predicate->node->region->graph;
	jive_gamma_normal_form * nf;
	nf = (jive_gamma_normal_form *) jive_graph_get_nodeclass_form(graph,
		&JIVE_GAMMA_NODE);
	
	if (nf->base.enable_mutable && nf->enable_predicate_reduction) {
		if (predicate->node->class_ == &JIVE_CONTROL_TRUE_NODE) {
			for (n = 0; n < nvalues; ++n)
				results[n] = true_values[n];
			return;
		} else if (predicate->node->class_ == &JIVE_CONTROL_FALSE_NODE) {
			for (n = 0; n < nvalues; ++n)
				results[n] = false_values[n];
			return;
		}
	}
	
	jive_output * tmp[nvalues * 2 + 1];
	tmp[0] = predicate;
	for (n = 0; n < nvalues; n++)
		tmp[n + 1] = false_values[n];
	for (n = 0; n < nvalues; n++)
		tmp[n + nvalues + 1] = true_values[n];
	jive_region * region = jive_region_innermost(nvalues * 2 + 1, tmp);
	
	jive_node * node = jive_gamma_create(region, predicate, nvalues, types, true_values, false_values);
	
	for (n = 0; n < nvalues; ++n)
		results[n] = node->outputs[n];
	
	if (nf->base.enable_mutable && nf->enable_invariant_reduction) {
		jive_node * true_branch = node->inputs[0]->origin->node;
		jive_node * false_branch = node->inputs[1]->origin->node;
		for (n = nvalues; n > 0; --n) {
			if (true_values[n-1] != false_values[n-1])
				continue;
			results[n-1] = true_values[n-1];
			jive_output_destroy(node->outputs[n-1]);
			jive_input_destroy(true_branch->inputs[n-1]);
			jive_input_destroy(false_branch->inputs[n-1]);
		}
	}
}

typedef struct jive_level_nodes jive_level_nodes;
struct jive_level_nodes {
	jive_node ** items;
	size_t nitems, space;
};

typedef struct jive_move_context jive_move_context;
struct jive_move_context {
	jive_level_nodes * depths;
	size_t max_depth_plus_one, space;
};

static void
jive_move_context_init(jive_move_context * self)
{
	self->depths = 0;
	self->max_depth_plus_one = 0;
	self->space = 0;
}

static void
jive_move_context_fini(jive_move_context * self, jive_context * context)
{
	size_t n;
	for (n = 0; n < self->max_depth_plus_one; n++)
		jive_context_free(context, self->depths[n].items);
	jive_context_free(context, self->depths);
}

static void
jive_level_nodes_append(jive_level_nodes * level, jive_context * context, jive_node * node)
{
	if (level->nitems == level->space) {
		level->space =  level->space * 2 + 1;
		level->items = jive_context_realloc(context, level->items, level->space * sizeof(jive_node *));
	}
	level->items[level->nitems ++] = node;
}

static void
jive_move_context_append(jive_move_context * self, jive_context * context, jive_node * node)
{
	if (node->depth_from_root >= self->space) {
		size_t new_space = self->space * 2;
		if (new_space <= node->depth_from_root)
			new_space = node->depth_from_root + 1;
		self->depths = jive_context_realloc(context, self->depths, new_space * sizeof(self->depths[0]));
		size_t n;
		for (n = self->space; n < new_space; n++) {
			self->depths[n].items = 0;
			self->depths[n].space = 0;
			self->depths[n].nitems = 0;
		}
		self->space = new_space;
	}
	jive_level_nodes_append(&self->depths[node->depth_from_root], context, node);
	if (node->depth_from_root + 1 > self->max_depth_plus_one)
		self->max_depth_plus_one = node->depth_from_root + 1;
}

static void
pre_move_region(jive_region * target_region, const jive_region * original_region,
	jive_move_context * move_context, jive_context * context)
{
	jive_node * node;
	JIVE_LIST_ITERATE(original_region->nodes, node, region_nodes_list) {
		if (node != original_region->bottom)
			jive_move_context_append(move_context, context, node);
	}
}

void
jive_region_move(const jive_region * self, jive_region * target)
{
	jive_context * context = target->graph->context;
	jive_move_context move_context;
	jive_move_context_init(&move_context);
	
	pre_move_region(target, self, &move_context, context);
	
	size_t depth;
	for (depth = 0; depth < move_context.max_depth_plus_one; depth ++) {
		size_t n;
		for (n = 0; n < move_context.depths[depth].nitems; n++) {
			jive_node * node = move_context.depths[depth].items[n];
			jive_node_move(node, target);
		}
	}
	
	jive_move_context_fini(&move_context, context);
}

/* normal forms */

const jive_gamma_normal_form_class JIVE_GAMMA_NORMAL_FORM_ = {
	.base = {
		.parent = 0,
		.fini = jive_node_normal_form_fini_,
		.normalize_node = jive_gamma_normal_form_normalize_node_,
		.operands_are_normalized = jive_gamma_normal_form_operands_are_normalized_,
		.set_mutable = jive_node_normal_form_set_mutable_,
		.set_cse = jive_node_normal_form_set_cse_
	},
	.set_predicate_reduction = jive_gamma_normal_form_class_set_predicate_reduction_,
	.set_invariant_reduction = jive_gamma_normal_form_class_set_invariant_reduction_,
};

bool
jive_gamma_normal_form_normalize_node_(const jive_node_normal_form * self_, jive_node * node)
{
	const jive_gamma_normal_form * self = (const jive_gamma_normal_form *) self_;
	
	if (!self->base.enable_mutable)
		return true;
	
	JIVE_DEBUG_ASSERT(node->noperands == 3);
	
	if (self->enable_predicate_reduction) {
		jive_output * pred = node->inputs[2]->origin;
		jive_node * branch = 0;
		if (pred->node->class_ == &JIVE_CONTROL_TRUE_NODE)
			branch = node->inputs[0]->origin->node;
		else if (pred->node->class_ == &JIVE_CONTROL_FALSE_NODE)
			branch = node->inputs[1]->origin->node;
		
		if (!branch)
			return true;
		
		jive_region_move(branch->region, node->region);
		size_t n;
		for (n = 0; n < node->noutputs; n++) {
			jive_output_replace(node->outputs[n], branch->inputs[n]->origin);
		}
		
		return false;
	}
	
	if (self->enable_invariant_reduction) {
		jive_node * true_branch = node->inputs[0]->origin->node;
		jive_node * false_branch = node->inputs[1]->origin->node;
		size_t n;
		for (n = node->noutputs; n > 0; --n) {
			if (true_branch->inputs[n-1]->origin != false_branch->inputs[n-1]->origin)
				continue;
			jive_output_replace(node->outputs[n-1], true_branch->inputs[n-1]->origin);
			jive_output_destroy(node->outputs[n-1]);
			jive_input_destroy(true_branch->inputs[n-1]);
			jive_input_destroy(false_branch->inputs[n-1]);
		}
	}
	
	return true;
}

bool
jive_gamma_normal_form_operands_are_normalized_(const jive_node_normal_form * self_,
	size_t noperands, jive_output * const operands[],
	const jive_node_attrs * attrs)
{
	const jive_gamma_normal_form * self = (const jive_gamma_normal_form *) self_;
	
	if (!self->base.enable_mutable)
		return true;
	
	JIVE_DEBUG_ASSERT(noperands == 3);
	
	if (self->enable_predicate_reduction) {
		jive_output * pred = operands[2];
		if (pred->node->class_ == &JIVE_CONTROL_TRUE_NODE)
			return false;
		if (pred->node->class_ == &JIVE_CONTROL_FALSE_NODE)
			return false;
	}
	
	if (self->enable_invariant_reduction) {
		jive_node * true_branch = operands[0]->node;
		jive_node * false_branch = operands[1]->node;
		size_t n;
		for (n = true_branch->ninputs; n > 0; --n) {
			if (true_branch->inputs[n-1]->origin == false_branch->inputs[n-1]->origin)
				return false;
		}
	}
	
	return true;
}

void
jive_gamma_normal_form_init_(jive_gamma_normal_form * self,
	const jive_node_class * cls, jive_node_normal_form * parent_,
	jive_graph * graph)
{
	jive_node_normal_form_init_(&self->base, cls, parent_, graph);
	
	jive_gamma_normal_form * parent = jive_gamma_normal_form_cast(parent_);
	
	if (parent) {
		self->enable_predicate_reduction = parent->enable_predicate_reduction;
		self->enable_invariant_reduction = parent->enable_invariant_reduction;
	} else {
		self->enable_predicate_reduction = true;
		self->enable_invariant_reduction = true;
	}
}

void
jive_gamma_normal_form_class_set_predicate_reduction_(jive_gamma_normal_form * self, bool enable)
{
	if (self->enable_predicate_reduction == enable)
		return;
	
	self->enable_predicate_reduction = enable;
	jive_node_normal_form * child;
	JIVE_LIST_ITERATE(self->base.subclasses, child, normal_form_subclass_list) {
		jive_gamma_normal_form_set_predicate_reduction((jive_gamma_normal_form *)child, enable);
	}
	if (enable && self->enable_predicate_reduction)
		jive_graph_mark_denormalized(self->base.graph);
}

void
jive_gamma_normal_form_class_set_invariant_reduction_(jive_gamma_normal_form * self, bool enable)
{
	if (self->enable_invariant_reduction == enable)
		return;
	
	self->enable_invariant_reduction = enable;
	jive_node_normal_form * child;
	JIVE_LIST_ITERATE(self->base.subclasses, child, normal_form_subclass_list) {
		jive_gamma_normal_form_set_invariant_reduction((jive_gamma_normal_form *)child, enable);
	}
	if (enable && self->enable_invariant_reduction)
		jive_graph_mark_denormalized(self->base.graph);
}
