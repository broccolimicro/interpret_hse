#pragma once

#include <hse/graph.h>

namespace hse {

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	boolean::cover cond;
};

segment compose(hse::graph &dst, petri::Composition composition, segment s0, segment s1);

}
