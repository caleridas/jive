#include <assert.h>
#include <stdio.h>
#include <locale.h>
#include <jive/vsdg.h>
#include <jive/view.h>
#include <jive/regalloc/shaping-traverser.h>

#include <jive/vsdg/node-private.h>

int main()
{
	setlocale(LC_ALL, "");
	jive_context * ctx = jive_context_create();
	jive_graph * graph = jive_graph_create(ctx);
	
	jive_region * region = graph->root_region;
	
	JIVE_DECLARE_TYPE(type);
	JIVE_DECLARE_CONTROL_TYPE(ctrl);
	
	jive_node * n1 = jive_node_create(region,
		0, NULL, NULL,
		1, (const jive_type *[]){type});
		
	jive_region * sub = jive_region_create_subregion(region);
	
	jive_node * n2 = jive_node_create(sub,
		0, NULL, NULL,
		1, (const jive_type *[]){ctrl});
	
	jive_node * n3 = jive_node_create(region,
		2, (const jive_type *[]){type, ctrl},
		(jive_output *[]){n1->outputs[0], n2->outputs[0]},
		0, NULL);
	
	jive_node_reserve(n3);
	
	jive_shaping_region_traverser * regtrav = jive_shaping_region_traverser_create(graph);
	
	jive_traverser * root_trav = jive_shaping_region_traverser_enter_region(regtrav, region);
	assert(root_trav->frontier.first->node == n3);
	
	jive_traverser * sub_trav = jive_shaping_region_traverser_enter_region(regtrav, sub);
	assert(sub_trav->frontier.first == 0);
	
	jive_cut_append(jive_region_create_cut(region), n3);
	assert(root_trav->frontier.first->node == n1);
	assert(sub_trav->frontier.first->node == n2);
	
	jive_shaping_region_traverser_destroy(regtrav);
	
	jive_graph_destroy(graph);
	assert(jive_context_is_empty(ctx));
	jive_context_destroy(ctx);
	return 0;
}
