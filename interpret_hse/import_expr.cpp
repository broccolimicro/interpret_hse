#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

hse::graph import_hse(const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define)
{
	hse::graph result;

	hse::transition n;
	if (assume) {
		n.assume = boolean::import_cover(syntax, result, default_id, tokens, auto_define);
	} else {
		n.guard = boolean::import_cover(syntax, result, default_id, tokens, auto_define);
	}

	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(n);
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(hse::state(vector<petri::token>(1, petri::token(b.index)), boolean::cube(1)));
	result.sink.push_back(hse::state(vector<petri::token>(1, petri::token(e.index)), boolean::cube(1)));
	return result;
}

hse::graph import_hse(const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(1, 1, boolean::import_cover(syntax, result, default_id, tokens, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(hse::state(vector<petri::token>(1, petri::token(b.index)), boolean::cube(1)));
	result.sink.push_back(hse::state(vector<petri::token>(1, petri::token(e.index)), boolean::cube(1)));
	return result;
}

}
