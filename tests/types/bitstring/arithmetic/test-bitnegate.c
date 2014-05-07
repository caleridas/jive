/*
 * Copyright 2010 2011 2012 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.h"

#include <locale.h>
#include <assert.h>

#include <jive/vsdg.h>
#include <jive/view.h>
#include <jive/types/bitstring.h>
#include <jive/vsdg/node-private.h>

static int test_main(void)
{
	setlocale(LC_ALL, "");

	jive_context * context = jive_context_create();
	jive_graph * graph = jive_graph_create(context);

	jive_bitstring_type bits32(32);
	const jive_type * tmparray11[] = {&bits32};
	jive_node * top = jive_node_create(graph->root_region,
		0, NULL, NULL,
		1, tmparray11);

	jive_output * c0 = jive_bitconstant_unsigned(graph, 32, 3);

	jive_output * neg0 = jive_bitnegate(top->outputs[0]);
	jive_output * neg1 = jive_bitnegate(c0);
	jive_output * neg2 = jive_bitnegate(neg1);
	const jive_type * tmparray0[] = {&bits32, &bits32, &bits32};
	jive_output * tmparray1[] = {neg0, neg1, neg2};

	jive_node * bottom = jive_node_create(graph->root_region,
		3, tmparray0, tmparray1,
		1, tmparray11);
	jive_graph_export(graph, bottom->outputs[0]);

	jive_graph_prune(graph);
	jive_view(graph, stdout);

	assert(jive_node_isinstance(bottom->inputs[0]->origin()->node, &JIVE_BITNEGATE_NODE));
	assert(jive_node_isinstance(bottom->inputs[1]->origin()->node, &JIVE_BITCONSTANT_NODE));
	assert(jive_node_isinstance(bottom->inputs[2]->origin()->node, &JIVE_BITCONSTANT_NODE));

	jive_bitconstant_node * bc1 = jive_bitconstant_node_cast(bottom->inputs[1]->origin()->node);
	jive_bitconstant_node * bc2 = jive_bitconstant_node_cast(bottom->inputs[2]->origin()->node);
	assert(jive_bitconstant_equals_signed(bc1, -3));
	assert(jive_bitconstant_equals_signed(bc2, 3));

	jive_graph_destroy(graph);
	assert(jive_context_is_empty(context));
	jive_context_destroy(context);

	return 0;
}

JIVE_UNIT_TEST_REGISTER("types/bitstring/arithmetic/test-bitnegate", test_main);
