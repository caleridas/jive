/*
 * Copyright 2010 2011 2012 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.h"

#include <locale.h>
#include <assert.h>
#include <stdint.h>

#include <jive/vsdg.h>
#include <jive/view.h>
#include <jive/vsdg/control.h>
#include <jive/vsdg/node-private.h>
#include <jive/types/bitstring/constant.h>
#include <jive/types/bitstring/comparison/bitslesseq.h>

static int test_main(void)
{
	setlocale(LC_ALL, "");

	jive_context * context = jive_context_create();
	jive_graph * graph = jive_graph_create(context);

	jive_bitstring_type bits32(32);
	const jive_type * tmparray0[] = {&bits32, &bits32};
	jive_node * top = jive_node_create(graph->root_region,
		0, NULL, NULL,
		2, tmparray0);

	jive_output * c0 = jive_bitconstant_signed(graph, 32, 4);
	jive_output * c1 = jive_bitconstant_signed(graph, 32, 5);
	jive_output * c2 = jive_bitconstant_signed(graph, 32, 0x7fffffffL);
	jive_output * c3 = jive_bitconstant_signed(graph, 32, (-0x7fffffffL-1));

	jive_output * slesseq0 = jive_bitslesseq(top->outputs[0], top->outputs[1]);
	jive_output * slesseq1 = jive_bitslesseq(c0, c1);
	jive_output * slesseq2 = jive_bitslesseq(c0, c0);
	jive_output * slesseq3 = jive_bitslesseq(c1, c0);
	jive_output * slesseq4 = jive_bitslesseq(top->outputs[0], c2);
	jive_output * slesseq5 = jive_bitslesseq(c3, top->outputs[1]);

	jive_control_type ctype;
	const jive_type * tmparray1[] = {&ctype, &ctype, &ctype, &ctype, &ctype, &ctype};
	jive_output * tmparray2[] = {slesseq0, slesseq1, slesseq2, slesseq3, slesseq4, slesseq5};
	jive_node * bottom = jive_node_create(graph->root_region,
		6, tmparray1,
		tmparray2, 1, tmparray0);
	jive_graph_export(graph, bottom->outputs[0]);

	jive_graph_prune(graph);
	jive_view(graph, stdout);

	assert(jive_node_isinstance(bottom->inputs[0]->origin()->node, &JIVE_BITSLESSEQ_NODE));
	assert(jive_node_isinstance(bottom->inputs[1]->origin()->node, &JIVE_CONTROL_TRUE_NODE));
	assert(jive_node_isinstance(bottom->inputs[2]->origin()->node, &JIVE_CONTROL_TRUE_NODE));
	assert(jive_node_isinstance(bottom->inputs[3]->origin()->node, &JIVE_CONTROL_FALSE_NODE));
	assert(jive_node_isinstance(bottom->inputs[4]->origin()->node, &JIVE_CONTROL_TRUE_NODE));
	assert(jive_node_isinstance(bottom->inputs[5]->origin()->node, &JIVE_CONTROL_TRUE_NODE));

	jive_graph_destroy(graph);
	assert(jive_context_is_empty(context));
	jive_context_destroy(context);

	return 0;
}


JIVE_UNIT_TEST_REGISTER("types/bitstring/comparison/test-bitslesseq", test_main);
