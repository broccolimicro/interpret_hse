#include "import_cog.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

segment import_segment(hse::graph &dst, const parse_cog::composition &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	bool arbiter = false;
	bool synchronizer = false;

	int composition = petri::parallel;
	if (syntax.level == parse_cog::composition::SEQUENCE or syntax.level == parse_cog::composition::INTERNAL_SEQUENCE) {
		composition = petri::sequence;
	} else if (syntax.level == parse_cog::composition::CONDITION) {
		composition = petri::choice;
	} else if (syntax.level == parse_cog::composition::CHOICE) {
		composition = petri::choice;
		arbiter = true;
	} else if (syntax.level == parse_cog::composition::PARALLEL) {
		composition = petri::parallel;
	}

	segment result(composition != petri::choice);
	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		segment branch(true);
		if (syntax.branches[i].sub != nullptr and syntax.branches[i].sub->valid) {
			branch = import_segment(dst, *syntax.branches[i].sub, default_id, tokens, auto_define);
		} else if (syntax.branches[i].ctrl != nullptr and syntax.branches[i].ctrl->valid) {
			branch = import_segment(dst, *syntax.branches[i].ctrl, default_id, tokens, auto_define);
		} else if (syntax.branches[i].assign.valid) {
			branch = import_segment(dst, syntax.branches[i].assign, default_id, tokens, auto_define);
		} else {
			continue;
		}
		result = compose(dst, composition, result, branch);
	}

	if (syntax.branches.size() == 0) {
		petri::iterator from = dst.create(hse::place());

		result.nodes.source.push_back({from});
		result.nodes.sink.push_back({from});
	}

	// At this point, source and sink are likely *transitions*.
	// If this is a conditional composition, then they need to be
	// the places *before* those transitions. And if there aren't
	// any places before those transitions then we need to create
	// them

	if (result.nodes.source.size() > 1u and composition == choice) {
		petri::iterator from = dst.create(hse::place());
		dst.connect({{from}}, result.nodes.source);
		result.nodes.source = petri::bound({{from}});
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

	if ((arbiter or synchronizer) and syntax.branches.size() > 1u) {
		for (auto i = result.nodes.source.begin(); i != result.nodes.source.end(); i++) {
			for (auto j = i->begin(); j != i->end(); j++) {
				if (j->type != place::type) {
					printf("%s:%d: internal: expected place\n", __FILE__, __LINE__);
				} else {
					if (arbiter) {
						dst.places[j->index].arbiter = true;
					}
					if (synchronizer) {
						dst.places[j->index].synchronizer = true;
					}
				}
			}
		}
	}

	if (result.loop and composition == choice and not arbiter and not result.nodes.source.empty()) {
		boolean::cover skipCond = ~result.cond;
		if (not skipCond.is_null()) {
			petri::iterator arrow = dst.create(hse::place());
			for (auto i = result.nodes.source.begin(); i != result.nodes.source.end(); i++) {
				petri::iterator skip = dst.create(hse::transition(1, skipCond));
				dst.connect(*i, skip);
				dst.connect(skip, arrow);
			}

			dst.connect(result.nodes.sink, {{arrow}});
			result.nodes.sink = petri::bound({{arrow}});
		}
		result.loop = false;
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

segment import_segment(hse::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	segment result(true);
	if (syntax.guard.valid and boolean::import_cover(syntax.guard, dst, default_id, tokens, auto_define) != 1) {
		segment sub = import_segment(dst, syntax.guard, syntax.kind == "assume", default_id, tokens, auto_define);
		result = compose(dst, petri::sequence, result, sub);
	}
	if (syntax.action.valid) {
		segment sub = import_segment(dst, syntax.action, default_id, tokens, auto_define);
		result = compose(dst, petri::sequence, result, sub);
	}

	if (syntax.kind == "while" and not result.nodes.source.empty()) {
		result.loop = true;
		petri::iterator link = dst.create(hse::place());
		dst.connect({{link}}, result.nodes.source);
		dst.connect(result.nodes.sink, {{link}});
		result.nodes.sink.clear();
		if (not result.nodes.reset.empty()) {
			result.nodes.source = result.nodes.reset;
			result.nodes.reset.clear();
		} else {
			result.nodes.source = petri::bound({{link}});
		}
	}

	return result;
}

void import_hse(hse::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define) {
	petri::segment result = hse::import_segment(dst, syntax, 0, tokens, auto_define).nodes;
	if (not result.reset.empty()) {
		result.source = result.reset;
		result.reset.clear();
	}

	vector<hse::state> reset;
	for (auto i = result.source.begin(); i != result.source.end(); i++) {
		hse::state rst;
		for (auto j = i->begin(); j != i->end(); j++) {
			if (j->type == hse::transition::type) {
				vector<petri::iterator> p = dst.prev(*j);
				if (p.empty()) {
					p.push_back(dst.create(place()));
					dst.connect(p.back(), *j);
				}
				for (auto k = p.begin(); k != p.end(); k++) {
					rst.tokens.push_back(petri::token(k->index));
				}
			} else {
				rst.tokens.push_back(petri::token(j->index));
			}
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
