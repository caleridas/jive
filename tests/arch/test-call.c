#include <locale.h>
#include <assert.h>

#include <jive/bitstring.h>
#include <jive/view.h>
#include <jive/arch/addresstype.h>
#include <jive/arch/address-transform.h>
#include <jive/arch/call.h>
#include <jive/vsdg/node-private.h>

int main()
{
	setlocale(LC_ALL, "");

	jive_context * context = jive_context_create();
	jive_graph * graph = jive_graph_create(context);

	JIVE_DECLARE_ADDRESS_TYPE(addr);
	JIVE_DECLARE_BITSTRING_TYPE(bits16, 16);
	jive_node * top = jive_node_create(graph->root_region,
		0, NULL, NULL,
		3, (const jive_type *[]){addr, bits16, addr});
	jive_node_reserve(top);

	jive_node * call = jive_call_by_address_node_create(graph->root_region,
		top->outputs[0], NULL,
		2, top->outputs + 1,
		3, (const jive_type *[]){bits16, addr, addr});

	jive_node * bottom = jive_node_create(graph->root_region,
		3, (const jive_type *[]){bits16, addr, addr}, call->outputs,
		0, NULL);
	jive_node_reserve(bottom);

	jive_view(graph, stdout);

	jive_call_node_address_transform(jive_call_node_cast(call), 32);

	jive_graph_prune(graph);
	jive_view(graph, stdout);
	
	jive_graph_destroy(graph);
	assert(jive_context_is_empty(context));
	jive_context_destroy(context);

	return 0;
}