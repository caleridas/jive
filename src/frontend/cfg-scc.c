/*
 * Copyright 2013 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/frontend/cfg.h>
#include <jive/frontend/cfg_node.h>
#include <jive/frontend/cfg-scc.h>
#include <jive/util/list.h>

#include <algorithm>
#include <unordered_map>

class index_item {
public:
	index_item(size_t index_, size_t lowlink_) noexcept : index(index_), lowlink(lowlink_) {}

	size_t index;
	size_t lowlink;
};

/* Tarjan's SCC algorithm */

static void
strongconnect(
	jive_cfg_node * node,
	std::unordered_map<jive_cfg_node*, std::unique_ptr<index_item>> & map,
	std::vector<jive_cfg_node*> & node_stack,
	size_t index,
	std::vector<std::unordered_set<jive_cfg_node*>> & sccs)
{
	map.emplace(node, std::unique_ptr<index_item>(new index_item(index, index)));
	node_stack.push_back(node);
	index++;

	jive_cfg_node * successor = node->taken_successor();
	if (successor != nullptr) {
		if (map.find(successor) == map.end()) {
			/* taken successor has not been visited yet; recurse on it */
			strongconnect(successor, map, node_stack, index, sccs);
			map[node]->lowlink = std::min(map[node]->lowlink, map[successor]->lowlink);
		} else if (std::find(node_stack.begin(), node_stack.end(), successor) != node_stack.end()) {
			/* taken successor is in stack and hence in the current SCC */
			map[node]->lowlink = std::min(map[node]->lowlink, map[successor]->index);
		}
	}

	successor = node->nottaken_successor();
	if (successor != nullptr) {
		if (map.find(successor) == map.end()) {
			/* nottaken successor has not been visited yet; recurse on it */
			strongconnect(successor, map, node_stack, index, sccs);
			map[node]->lowlink = std::min(map[node]->lowlink, map[successor]->lowlink);
		} else if (std::find(node_stack.begin(), node_stack.end(), successor) != node_stack.end()) {
			/* nottaken successor is in stack and hence in the current SCC */
			map[node]->lowlink = std::min(map[node]->lowlink, map[successor]->index);
		}
	}

	if (map[node]->lowlink == map[node]->index) {
		std::unordered_set<jive_cfg_node*> scc;
		jive_cfg_node * w;
		do {
			w = node_stack.back();
			node_stack.pop_back();
			scc.insert(w);
		} while (w != node);
		sccs.push_back(scc);
	}
}

std::vector<std::unordered_set<jive_cfg_node*>>
jive_cfg_find_sccs(jive_cfg * cfg)
{
	std::vector<std::unordered_set<jive_cfg_node*>> sccs;

	std::unordered_map<jive_cfg_node*, std::unique_ptr<index_item>> map;
	std::vector<jive_cfg_node*> node_stack;
	size_t index = 0;

	/* find strongly connected components */
	jive_cfg_node * cfg_node;
	JIVE_LIST_ITERATE(cfg->nodes, cfg_node, cfg_node_list) {
		if (map.find(cfg_node) == map.end())
			strongconnect(cfg_node, map, node_stack, index, sccs);
	}

	return sccs;
}
