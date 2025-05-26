#include "import_cog.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

petri::segment import_segment(hse::graph &dst, const parse_cog::composition &syntax, boolean::cover &covered, bool &hasRepeat, int default_id, tokenizer *tokens, bool auto_define) {
	petri::segment result;

	bool arbiter = false;
	bool synchronizer = false;

	int composition = hse::parallel;
	if (syntax.level == parse_cog::composition::SEQUENCE or syntax.level == parse_cog::composition::INTERNAL_SEQUENCE) {
		composition = hse::sequence;
	} else if (syntax.level == parse_cog::composition::CONDITION) {
		composition = hse::choice;
	} else if (syntax.level == parse_cog::composition::CHOICE) {
		composition = hse::choice;
		arbiter = true;
	} else if (syntax.level == parse_cog::composition::PARALLEL) {
		composition = hse::parallel;
	}

	bool subRepeat = false;
	covered = (composition == hse::choice ? 0 : 1);
	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		if (syntax.branches[i].sub != nullptr and syntax.branches[i].sub->valid) {
			boolean::cover subcovered;
			result = dst.compose(composition, result, import_segment(dst, *syntax.branches[i].sub, subcovered, subRepeat, default_id, tokens, auto_define));
			if (composition == hse::choice) {
				covered |= subcovered;
			} else if (composition == hse::parallel) {
				covered &= subcovered;
			} 
		} else if (syntax.branches[i].ctrl != nullptr and syntax.branches[i].ctrl->valid) {
			if (syntax.branches[i].ctrl->kind == "while") {
				subRepeat = true;
			}
			result = dst.compose(composition, result, import_segment(dst, *syntax.branches[i].ctrl, default_id, tokens, auto_define));
			boolean::cover subcovered = 1;
			if (syntax.branches[i].ctrl->guard.valid) {
				subcovered = boolean::import_cover(syntax.branches[i].ctrl->guard, dst, default_id, tokens, auto_define);
			}
			if (composition == hse::choice) {
				covered |= subcovered;
			} else if (composition == hse::parallel) {
				covered &= subcovered;
			}
		} else if (syntax.branches[i].assign.valid) {
			result = dst.compose(composition, result, import_segment(dst, syntax.branches[i].assign, default_id, tokens, auto_define));
			if (composition == hse::choice) {
				covered = 1;
			}
		}
	}

	if (syntax.branches.size() == 0) {
		petri::iterator b = dst.create(hse::place());

		result.source.push_back({b});
		result.sink.push_back({b});
	}

	/*if ((int)syntax.branches.size() > 1) {
		cout << "BEFORE" << endl;
		cout << syntax.to_string() << endl << endl;
		cout << "Source: ";
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;
		cout << "Sink: ";
		for (auto i = result.sink.begin(); i != result.sink.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << export_astg(result).to_string() << endl << endl;
	}*/

	if (result.source.size() > 1 and composition == choice) {
		result.source = petri::bound({{dst.nest_in(result.source)}});
	}

	if ((arbiter or synchronizer) and (int)syntax.branches.size() > 1) {
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->begin(); j != i->end(); j++) {
				if (arbiter) {
					dst.places[j->index].arbiter = true;
				}
				if (synchronizer) {
					dst.places[j->index].synchronizer = true;
				}
			}
		}
	}

	if (subRepeat and composition == choice and not arbiter and not result.source.empty()) {
		boolean::cover skipCond = ~covered;
		if (not skipCond.is_null()) {
			petri::iterator arrow = dst.create(hse::place());
			for (auto i = result.source.begin(); i != result.source.end(); i++) {
				petri::iterator skip = dst.create(hse::transition(1, skipCond));
				for (auto j = i->begin(); j != i->end(); j++) {
					dst.connect(*j, skip);
				}
				dst.connect(skip, arrow);
			}

			for (auto i = result.sink.begin(); i != result.sink.end(); i++) {
				petri::iterator skip = dst.create(hse::transition(1, 1));
				for (auto j = i->begin(); j != i->end(); j++) {
					dst.connect(*j, skip);
				}
				dst.connect(skip, arrow);
			}

			result.sink = petri::bound({{arrow}});
		}
		subRepeat = false;
	}

	if (subRepeat) {
		hasRepeat = true;
	}

	/*if ((int)syntax.branches.size() > 1) {
		cout << "AFTER" << endl;
		cout << "Source: ";
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;
		cout << "Sink: ";
		for (auto i = result.sink.begin(); i != result.sink.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << export_astg(result).to_string() << endl << endl;
	}*/

	return result;
}

petri::segment import_segment(hse::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;

	if (syntax.guard.valid and boolean::import_cover(syntax.guard, dst, default_id, tokens, auto_define) != 1) {
		result = dst.compose(hse::sequence, result, import_segment(dst, syntax.guard, syntax.kind == "assume", default_id, tokens, auto_define));
	}
	if (syntax.action.valid) {
		boolean::cover covered;
		bool hasRepeat = false;
		result = dst.compose(hse::sequence, result, import_segment(dst, syntax.action, covered, hasRepeat, default_id, tokens, auto_define));
	}

	if (syntax.kind == "while" and not result.source.empty()) {
		dst.connect(result.sink, result.source);
		result.sink.clear();
		if (result.reset.size() > 0) {
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}

void import_hse(hse::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define) {
	boolean::cover covered;
	bool hasRepeat = false;
	petri::segment result = hse::import_segment(dst, syntax, covered, hasRepeat, 0, tokens, auto_define);
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
