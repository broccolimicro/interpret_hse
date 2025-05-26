#include "import_expr.h"

#include <common/standard.h>
#include <petri/iterator.h>
#include <interpret_boolean/import.h>

namespace hse {

petri::segment import_segment(hse::graph &dst, const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define) {
	hse::transition n;
	if (assume) {
		n.assume = boolean::import_cover(syntax, dst, default_id, tokens, auto_define);
	} else {
		n.guard = boolean::import_cover(syntax, dst, default_id, tokens, auto_define);
	}

	hse::iterator t = dst.create(n);
	return petri::segment(petri::bound({{t}}), petri::bound({{t}}));
}

petri::segment import_segment(hse::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	hse::iterator t = dst.create(hse::transition(1, 1, boolean::import_cover(syntax, dst, default_id, tokens, auto_define)));
	return petri::segment(petri::bound({{t}}), petri::bound({{t}}));
}

}
