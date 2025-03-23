#include "import_chp.h"
#include "import_expr.h"
#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

hse::graph import_hse(const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define)
{
	if (syntax.region != "")
		default_id = atoi(syntax.region.c_str());

	hse::graph result;

	int composition = hse::parallel;
	if (parse_chp::composition::precedence[syntax.level] == "||" || parse_chp::composition::precedence[syntax.level] == ",")
		composition = hse::parallel;
	else if (parse_chp::composition::precedence[syntax.level] == ";")
		composition = hse::sequence;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		if (syntax.branches[i].sub.valid)
			result.merge(composition, import_hse(syntax.branches[i].sub, default_id, tokens, auto_define));
		else if (syntax.branches[i].ctrl.valid)
			result.merge(composition, import_hse(syntax.branches[i].ctrl, default_id, tokens, auto_define));
		else if (syntax.branches[i].assign.valid)
			result.merge(composition, import_hse(syntax.branches[i].assign, default_id, tokens, auto_define));

		if (syntax.reset == 0 && i == 0)
			result.reset = result.source;
		else if (syntax.reset == i+1)
			result.reset = result.sink;
	}

	if (syntax.branches.size() == 0)
	{
		petri::iterator b = result.create(hse::place());

		result.source.push_back(hse::state(vector<hse::token>(1, hse::token(b.index)), boolean::cube()));
		result.sink.push_back(hse::state(vector<hse::token>(1, hse::token(b.index)), boolean::cube()));

		if (syntax.reset >= 0)
			result.reset = result.source;
	}

	return result;
}

hse::graph import_hse(const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define)
{
	if (syntax.region != "")
		default_id = atoi(syntax.region.c_str());

	hse::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid and boolean::import_cover(syntax.branches[i].first, result, default_id, tokens, auto_define) != 1)
			branch.merge(hse::sequence, import_hse(syntax.branches[i].first, syntax.assume, default_id, tokens, auto_define));
		if (syntax.branches[i].second.valid)
			branch.merge(hse::sequence, import_hse(syntax.branches[i].second, default_id, tokens, auto_define));

		result.merge(hse::choice, branch);
	}

	if ((not syntax.deterministic or syntax.repeat) and not syntax.branches.empty())
	{
		hse::iterator sm = result.create(hse::place());
		for (int i = 0; i < (int)result.source.size(); i++)
		{
			if (result.source[i].tokens.size() > 1)
			{
				petri::iterator split_t = result.create(hse::transition());
				result.connect(sm, split_t);

				for (int j = 0; j < (int)result.source[i].tokens.size(); j++)
					result.connect(split_t, petri::iterator(hse::place::type, result.source[i].tokens[j].index));
			}
			else if (result.source[i].tokens.size() == 1)
			{
				petri::iterator loc(hse::place::type, result.source[i].tokens[0].index);
				result.connect(result.prev(loc), sm);
				result.connect(sm, result.next(loc));
				result.places[sm.index].arbiter = (result.places[sm.index].arbiter or result.places[loc.index].arbiter);
				result.places[sm.index].synchronizer = (result.places[sm.index].synchronizer or result.places[loc.index].synchronizer);
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.source.clear();
		result.source.push_back(hse::state(vector<petri::token>(1, petri::token(sm.index)), boolean::cube(1)));
	}

	if (not syntax.deterministic) {
		for (int i = 0; i < (int)result.source.size(); i++) {
			for (int j = 0; j < (int)result.source[i].tokens.size(); j++) {
				if (not syntax.stable) {
					result.places[result.source[i].tokens[j].index].synchronizer = true;
				} else {
					result.places[result.source[i].tokens[j].index].arbiter = true;
				}
			}
		}
	}

	if (syntax.repeat && syntax.branches.size() > 0)
	{
		hse::iterator sm(hse::place::type, result.source[0].tokens[0].index);
		for (int i = 0; i < (int)result.sink.size(); i++)
		{
			if (result.sink[i].tokens.size() > 1)
			{
				petri::iterator merge_t = result.create(hse::transition());
				result.connect(merge_t, sm);

				for (int j = 0; j < (int)result.sink[i].tokens.size(); j++)
					result.connect(petri::iterator(hse::place::type, result.sink[i].tokens[j].index), merge_t);
			}
			else if (result.sink[i].tokens.size() == 1)
			{
				petri::iterator loc(hse::place::type, result.sink[i].tokens[0].index);
				result.connect(result.prev(loc), sm);
				result.connect(sm, result.next(loc));
				result.places[sm.index].arbiter = (result.places[sm.index].arbiter or result.places[loc.index].arbiter);
				result.places[sm.index].synchronizer = (result.places[sm.index].synchronizer or result.places[loc.index].synchronizer);
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.sink.clear();

		boolean::cover repeat = 1;
		for (int i = 0; i < (int)syntax.branches.size() && !repeat.is_null(); i++)
		{
			if (syntax.branches[i].first.valid)
			{
				if (i == 0)
					repeat = ~boolean::import_cover(syntax.branches[i].first, result, default_id, tokens, auto_define);
				else
					repeat &= ~boolean::import_cover(syntax.branches[i].first, result, default_id, tokens, auto_define);
			}
			else
			{
				repeat = 0;
				break;
			}
		}

		if (!repeat.is_null())
		{
			hse::iterator guard = result.create(hse::transition(1, repeat));
			result.connect(sm, guard);
			hse::iterator arrow = result.create(hse::place());
			result.connect(guard, arrow);

			result.sink.push_back(hse::state(vector<petri::token>(1, petri::token(arrow.index)), boolean::cube(1)));
		}

		if (result.reset.size() > 0)
		{
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}

}
