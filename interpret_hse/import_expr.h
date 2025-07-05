#pragma once

#include <hse/graph.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace hse {

struct segment {
	segment(bool cond);
	~segment();

	petri::segment nodes;
	bool loop;
	boolean::cover cond;
};

segment compose(hse::graph &dst, int composition, segment s0, segment s1);

segment import_segment(hse::graph &dst, const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define);
segment import_segment(hse::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
