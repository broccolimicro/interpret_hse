#pragma once

#include <hse/graph.h>

#include <interpret_boolean/import.h>

namespace hse {

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	boolean::cover cond;
};

segment compose(hse::graph &dst, int composition, segment s0, segment s1);

template <int group, typename number_t, typename instance_t>
segment import_segment(hse::graph &dst, const parse_expression::expression_t<group, number_t, instance_t> &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define) {
	segment result(false);
	hse::transition n;
	if (assume) {
		n.assume = boolean::import_cover(syntax, dst, default_id, tokens, auto_define);
	} else {
		n.guard = boolean::import_cover(syntax, dst, default_id, tokens, auto_define);
		result.cond = n.guard;
	}

	hse::iterator t = dst.create(n);
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

template <int group, typename number_t, typename instance_t>
segment import_segment(hse::graph &dst, const parse_expression::assignment_t<group, number_t, instance_t> &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	segment result(true);
	petri::iterator t = dst.create(hse::transition(1, 1, boolean::import_cover(syntax, dst, default_id, tokens, auto_define)));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

}
