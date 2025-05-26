#include "import_chp.h"
#include "import_expr.h"
#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

petri::segment import_segment(hse::graph &dst, const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;

	int composition = hse::parallel;
	if (parse_chp::composition::precedence[syntax.level] == "||" or parse_chp::composition::precedence[syntax.level] == ",") {
		composition = hse::parallel;
	} else if (parse_chp::composition::precedence[syntax.level] == ";") {
		composition = hse::sequence;
	}

	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		petri::segment branch;
		if (syntax.branches[i].sub.valid) {
			branch = import_segment(dst, syntax.branches[i].sub, default_id, tokens, auto_define);
		} else if (syntax.branches[i].ctrl.valid) {
			branch = import_segment(dst, syntax.branches[i].ctrl, default_id, tokens, auto_define);
		} else if (syntax.branches[i].assign.valid) {
			branch = import_segment(dst, syntax.branches[i].assign, default_id, tokens, auto_define);
		}

		result = dst.compose(composition, result, branch);

		if (syntax.reset == 0 and i == 0) {
			result.reset = result.source;
		} else if (syntax.reset == i+1) {
			result.reset = result.sink;
		}
	}

	if (syntax.branches.size() == 0) {
		petri::iterator b = dst.create(hse::place());

		result.source = petri::bound({{b}});
		result.sink = petri::bound({{b}});

		if (syntax.reset >= 0) {
			result.reset = result.source;
		}
	}

	return result;
}

petri::segment import_segment(hse::graph &dst, const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;
	if (syntax.branches.empty()) {
		return result;
	}

	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		petri::segment branch;
		if (syntax.branches[i].first.valid and boolean::import_cover(syntax.branches[i].first, dst, default_id, tokens, auto_define) != 1) {
			branch = dst.compose(hse::sequence, branch, import_segment(dst, syntax.branches[i].first, syntax.assume, default_id, tokens, auto_define));
		}
		if (syntax.branches[i].second.valid) {
			branch = dst.compose(hse::sequence, branch, import_segment(dst, syntax.branches[i].second, default_id, tokens, auto_define));
		}
		result = dst.compose(hse::choice, result, branch);
	}

	if (not syntax.deterministic or syntax.repeat) {
		hse::iterator p = dst.nest_in(result.source);
		result.source = petri::bound({{p}});
		if (not syntax.deterministic) {
			for (auto i = result.source.begin(); i != result.source.end(); i++) {
				for (auto j = i->begin(); j != i->end(); j++) {
					if (not syntax.stable) {
						dst.places[j->index].synchronizer = true;
					} else {
						dst.places[j->index].arbiter = true;
					}
				}
			}
		}
	}

	if (syntax.repeat) {
		dst.connect(result.sink, result.source);
		result.sink.clear();

		boolean::cover repeat = 1;
		for (int i = 0; i < (int)syntax.branches.size() and not repeat.is_null(); i++) {
			if (syntax.branches[i].first.valid) {
				if (i == 0) {
					repeat = ~boolean::import_cover(syntax.branches[i].first, dst, default_id, tokens, auto_define);
				} else {
					repeat &= ~boolean::import_cover(syntax.branches[i].first, dst, default_id, tokens, auto_define);
				}
			} else {
				repeat = 0;
				break;
			}
		}

		if (not repeat.is_null()) {
			hse::iterator guard = dst.create(hse::transition(1, repeat));
			dst.connect(result.source, petri::bound({{guard}}));
			hse::iterator arrow = dst.create(hse::place());
			dst.connect(guard, arrow);

			result.sink = petri::bound({{arrow}});
		}

		if (not result.reset.empty()) {
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}

void import_hse(hse::graph &dst, const parse_chp::composition &syntax, tokenizer *tokens, bool auto_define) {
	petri::segment result = import_segment(dst, syntax, 0, tokens, auto_define);
	if (not result.reset.empty()) {
		result.source = result.reset;
		result.reset.clear();
	}

	vector<hse::state> reset;
	for (auto i = result.source.begin(); i != result.source.end(); i++) {
		hse::state rst;
		for (auto j = i->begin(); j != i->end(); j++) {
			if (j->type == hse::transition::type) {
				petri::iterator p = dst.create(place());
				dst.connect(p, *j);
				*j = p;
			}
			rst.tokens.push_back(petri::token(j->index));
		}
		reset.push_back(rst);
	}

	if (dst.reset.empty()) {
		dst.reset = reset;
	} else {
		vector<hse::state> prev = dst.reset;
		dst.reset.clear();

		for (auto i = prev.begin(); i != prev.end(); i++) {
			for (auto j = reset.begin(); j != reset.end(); j++) {
				dst.reset.push_back(state::merge(*i, *j));
			}
		}
	}
}

}
