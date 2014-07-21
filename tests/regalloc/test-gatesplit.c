/*
 * Copyright 2010 2011 2012 2013 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.h"

#include <assert.h>
#include <locale.h>
#include <stdio.h>

#include <jive/arch/subroutine/nodes.h>
#include <jive/regalloc.h>
#include <jive/regalloc/shaped-graph.h>
#include <jive/view.h>
#include <jive/vsdg.h>

#include "testarch.h"

static jive_graph *
create_testgraph_gatesplit(jive_context * context)
{
	jive_graph * graph = jive_graph_create(context);
	jive_subroutine subroutine = jive_testarch_subroutine_begin(graph,
		0, NULL,
		0, NULL);
	
	jive::output * memstate = jive_subroutine_simple_get_global_state(subroutine);
	const jive::base::type * memtype = &memstate->type();
	jive_node * enter_mux = jive_state_split(memtype, memstate, 1)[0]->node();
	jive_node * leave_mux = jive_state_merge(memtype, 1, enter_mux->outputs)->node();
	jive_subroutine_simple_set_global_state(subroutine, leave_mux->outputs[0]);
	
	jive::gate * var1_gate = jive_register_class_create_gate(&jive_testarch_regcls_gpr, graph, "var1");
	jive::gate * var2_gate = jive_register_class_create_gate(&jive_testarch_regcls_gpr, graph, "var2");
	jive::gate * var3_gate = jive_register_class_create_gate(&jive_testarch_regcls_gpr, graph, "var3");
	
	jive_node *n1 = jive_instruction_node_create(
		subroutine.region,
		&JIVE_PSEUDO_NOP,
		NULL, NULL);
	jive_node *n2 = jive_instruction_node_create(
		subroutine.region,
		&JIVE_PSEUDO_NOP,
		NULL, NULL);
	
	jive_node_gate_input(n1, var2_gate, jive_node_gate_output(enter_mux, var1_gate));
	jive_node_gate_input(n2, var3_gate, jive_node_gate_output(n1, var2_gate));
	jive_node_gate_input(leave_mux, var1_gate, jive_node_gate_output(n2, var3_gate));
	
	jive::gate * var_cls1 = jive_register_class_create_gate(&jive_testarch_regcls_r1, graph, "cls1");
	jive::gate * var_cls2 = jive_register_class_create_gate(&jive_testarch_regcls_r2, graph, "cls2");
	jive::gate * var_cls3 = jive_register_class_create_gate(&jive_testarch_regcls_r3, graph, "cls3");
	
	jive_node_gate_output(enter_mux, var_cls1);
	jive_node_gate_output(n1, var_cls2);
	jive_node_gate_output(n2, var_cls3);
	
	jive_graph_export(graph, jive_subroutine_end(subroutine)->outputs[0]);
	
	return graph;
}


typedef jive_graph * (*creator_function_t)(jive_context *);

static const creator_function_t tests[] = {
	create_testgraph_gatesplit
};

static int test_main(void)
{
	setlocale(LC_ALL, "");
	
	jive_context * context = jive_context_create();
	
	size_t n;
	for (n = 0; n < sizeof(tests)/sizeof(tests[0]); n++) {
		fprintf(stderr, "%zd\n", n);
		jive_graph * graph = tests[n](context);
		jive_view(graph, stdout);
		jive_shaped_graph * shaped_graph = jive_regalloc(graph);
		jive_view(graph, stdout);
		jive_shaped_graph_destroy(shaped_graph);
		jive_graph_destroy(graph);
	}
	
	assert(jive_context_is_empty(context));
	jive_context_destroy(context);
	
	return 0;
}

JIVE_UNIT_TEST_REGISTER("regalloc/test-gatesplit", test_main);
