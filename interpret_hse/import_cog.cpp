#include "import_cog.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

hse::graph import_hse(const parse_cog::composition &syntax, boolean::cover &covered, bool &hasRepeat, int default_id, tokenizer *tokens, bool auto_define) {
	hse::graph result;

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
	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		if (syntax.branches[i].sub != nullptr and syntax.branches[i].sub->valid) {
			boolean::cover subcovered;
			result.merge(composition, import_hse(*syntax.branches[i].sub, subcovered, subRepeat, default_id, tokens, auto_define));
			if (composition == hse::choice) {
				covered |= subcovered;
			} else if (composition == hse::parallel) {
				covered &= subcovered;
			} 
		} else if (syntax.branches[i].ctrl != nullptr and syntax.branches[i].ctrl->valid) {
			if (syntax.branches[i].ctrl->kind == "while") {
				subRepeat = true;
			}
			result.merge(composition, import_hse(*syntax.branches[i].ctrl, default_id, tokens, auto_define));
			boolean::cover subcovered = 1;
			if (syntax.branches[i].ctrl->guard.valid) {
				subcovered = boolean::import_cover(syntax.branches[i].ctrl->guard, result, default_id, tokens, auto_define);
			}
			if (composition == hse::choice) {
				covered |= subcovered;
			} else if (composition == hse::parallel) {
				covered &= subcovered;
			}
		} else if (syntax.branches[i].assign.valid) {
			result.merge(composition, import_hse(syntax.branches[i].assign, default_id, tokens, auto_define));
			if (composition == hse::choice) {
				covered = 1;
			}
		}
	}

	if (syntax.branches.size() == 0)
	{
		petri::iterator b = result.create(hse::place());

		result.source.push_back(hse::state(vector<hse::token>(1, hse::token(b.index)), boolean::cube()));
		result.sink.push_back(hse::state(vector<hse::token>(1, hse::token(b.index)), boolean::cube()));
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
		result.source = result.consolidate(result.source);
	}

	if ((arbiter or synchronizer) and (int)syntax.branches.size() > 1) {
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				if (arbiter) {
					result.places[j->index].arbiter = true;
				}
				if (synchronizer) {
					result.places[j->index].synchronizer = true;
				}
			}
		}
	}

	if (subRepeat and composition == choice and not arbiter and not result.source.empty()) {
		boolean::cover skipCond = ~covered;
		if (not skipCond.is_null()) {
			petri::iterator arrow = result.create(hse::place());
			for (auto i = result.source.begin(); i != result.source.end(); i++) {
				petri::iterator skip = result.create(hse::transition(1, skipCond));
				for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
					result.connect(petri::iterator(place::type, j->index), skip);
				}
				result.connect(skip, arrow);
			}

			for (auto i = result.sink.begin(); i != result.sink.end(); i++) {
				petri::iterator skip = result.create(hse::transition(1, 1));
				for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
					result.connect(petri::iterator(place::type, j->index), skip);
				}
				result.connect(skip, arrow);
			}

			result.sink = vector<hse::state>(1, hse::state(vector<hse::token>(1, hse::token(arrow.index)), boolean::cube(1)));
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

hse::graph import_hse(const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	hse::graph result;

	if (syntax.guard.valid and boolean::import_cover(syntax.guard, result, default_id, tokens, auto_define) != 1) {
		result.merge(hse::sequence, import_hse(syntax.guard, syntax.kind == "assume", default_id, tokens, auto_define));
	}
	if (syntax.action.valid) {
		boolean::cover covered;
		bool hasRepeat = false;
		result.merge(hse::sequence, import_hse(syntax.action, covered, hasRepeat, default_id, tokens, auto_define));
	}

	if (syntax.kind == "while" and not result.source.empty()) {
		result.consolidate(result.sink, result.source, true);

		result.sink.clear();
		if (result.reset.size() > 0) {
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}


}
