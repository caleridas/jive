/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2012 2013 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "test-registry.h"

#include <assert.h>

#include <jive/arch/addresstype.h>
#include <jive/arch/store.h>
#include <jive/rvsdg.h>
#include <jive/types/bitstring/type.h>
#include <jive/types/record.h>
#include <jive/types/union.h>
#include <jive/view.h>

#include "testnodes.h"

static int test_main(void)
{
	using namespace jive;

	jive::graph graph;

	memtype mt;
	addrtype at;
	auto rcddcl = rcddeclaration::create(&graph, {&bit8, &bit16, &bit32});
	auto unndcl = unndeclaration::create(&graph, {&bit8, &bit16, &bit32});
	auto eunndcl = unndeclaration::create(&graph);

	jive::rcdtype rcdtype(rcddcl);
	jive::unntype unntype(unndcl);
	jive::unntype eunntype(eunndcl);

	auto i0 = graph.add_import(at, "");
	auto i1 = graph.add_import(mt, "");
	auto i2 = graph.add_import(bit8, "");
	auto i3 = graph.add_import(bit16, "");
	auto i4 = graph.add_import(bit32, "");
	auto i5 = graph.add_import(mt, "");
	auto i6 = graph.add_import(at, "");

	auto states0 = addrstore_op::create(i0, i4, bit32, {i1});

	jive::output * tmparray1[] = {i2, i3, i4};
	auto group = jive_group_create(rcddcl, 3, tmparray1);
	auto states1 = addrstore_op::create(i0, group, rcdtype, {i1, i5});

	auto unify = jive_unify_create(unndcl, 2, i4);
	auto states2 = addrstore_op::create(i0, unify, unntype, {i1});

	auto states3 = addrstore_op::create(i6, i4, bit32, {i1});
	auto states4 = addrstore_op::create(i0, i4, bit32, {states3});

	unify = jive_empty_unify_create(graph.root(), eunndcl);
	auto states5 = addrstore_op::create(i0, unify, eunntype, {i1});

	graph.add_export(states0[0], "");
	graph.add_export(states1[0], "");
	graph.add_export(states2[0], "");
	graph.add_export(states4[0], "");
	graph.add_export(states5[0], "");

	graph.normalize();
	graph.prune();

	jive::view(graph.root(), stderr);

	assert(states3[0]->node()->input(2)->origin() == i1);

	return 0;
}

JIVE_UNIT_TEST_REGISTER("arch/test-store", test_main)
