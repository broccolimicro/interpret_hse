#include "import_expr.h"

#include <common/standard.h>
#include <petri/iterator.h>
#include <interpret_boolean/import.h>

namespace hse {

segment::segment(bool cond) {
	this->loop = false;
	this->cond = cond;
}

segment::~segment() {
}

segment compose(hse::graph &dst, int composition, segment s0, segment s1) {
	if (composition == petri::choice) {
		s0.cond = s0.cond | s1.cond;
	} else if (composition == petri::parallel) {
		s0.cond = s0.cond & s1.cond;
	} else if (composition == petri::sequence) {
		if (s0.nodes.source.empty()) {
			s0.cond = s1.cond;
			s0.loop = s1.loop;
		}
	}
	s0.nodes = dst.compose(composition, s0.nodes, s1.nodes);
	return s0;
}

segment import_segment(hse::graph &dst, const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define) {
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

segment import_segment(hse::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	segment result(true);
	petri::iterator t = dst.create(hse::transition(1, 1, boolean::import_cover(syntax, dst, default_id, tokens, auto_define)));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

}
