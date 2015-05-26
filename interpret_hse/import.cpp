/*
 * import.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "import.h"
#include <interpret_boolean/import.h>

hse::graph import_graph(tokenizer &tokens, const parse_boolean::disjunction &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(hse::transition::passive, import_cover(tokens, syntax, variables, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(vector<hse::local_token>(1, hse::local_token(b.index, boolean::cube())));
	result.sink.push_back(vector<hse::local_token>(1, hse::local_token(e.index, boolean::cube())));
	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_boolean::internal_choice &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(hse::transition::active, import_cover(tokens, syntax, variables, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(vector<hse::local_token>(1, hse::local_token(b.index, boolean::cube())));
	result.sink.push_back(vector<hse::local_token>(1, hse::local_token(e.index, boolean::cube())));
	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::sequence &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	for (int i = 0; i < (int)syntax.actions.size(); i++)
		if (syntax.actions[i] != NULL && syntax.actions[i]->valid)
		{
			if (syntax.actions[i]->is_a<parse_hse::parallel>())
				result.merge(hse::sequence, import_graph(tokens, *(parse_hse::parallel*)syntax.actions[i], variables, auto_define));
			else if (syntax.actions[i]->is_a<parse_hse::condition>())
				result.merge(hse::sequence, import_graph(tokens, *(parse_hse::condition*)syntax.actions[i], variables, auto_define));
			else if (syntax.actions[i]->is_a<parse_hse::loop>())
				result.merge(hse::sequence, import_graph(tokens, *(parse_hse::loop*)syntax.actions[i], variables, auto_define));
			else if (syntax.actions[i]->is_a<parse_boolean::internal_choice>())
				result.merge(hse::sequence, import_graph(tokens, *(parse_boolean::internal_choice*)syntax.actions[i], variables, auto_define));
		}

	if (syntax.actions.size() == 0)
	{
		hse::iterator b = result.create(hse::place());
		hse::iterator t = result.create(hse::transition());
		hse::iterator e = result.create(hse::place());

		result.connect(b, t);
		result.connect(t, e);

		result.source.push_back(vector<hse::local_token>(1, hse::local_token(b.index, boolean::cube())));
		result.sink.push_back(vector<hse::local_token>(1, hse::local_token(e.index, boolean::cube())));
	}

	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::parallel &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
		if (syntax.branches[i].valid)
			result.merge(hse::parallel, import_graph(tokens, syntax.branches[i], variables, auto_define));

	result.wrap();

	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::condition &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid)
			branch.merge(hse::sequence, import_graph(tokens, syntax.branches[i].first, variables, auto_define));
		if (syntax.branches[i].second.valid)
			branch.merge(hse::sequence, import_graph(tokens, syntax.branches[i].second, variables, auto_define));
		result.merge(hse::choice, branch);
	}

	result.wrap();

	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::loop &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid)
			branch.merge(hse::sequence, import_graph(tokens, syntax.branches[i].first, variables, auto_define));
		if (syntax.branches[i].second.valid)
			branch.merge(hse::sequence, import_graph(tokens, syntax.branches[i].second, variables, auto_define));
		result.merge(hse::choice, branch);
	}

	boolean::cover repeat = 0;
	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		if (syntax.branches[i].first.valid)
			repeat |= import_cover(tokens, syntax.branches[i].first, variables, auto_define);
		else
			repeat = 1;
	}

	hse::iterator sm = result.create(hse::place());
	for (int i = 0; i < (int)result.source.size(); i++)
	{
		boolean::cube state;
		for (int j = 0; j < (int)result.source[i].size(); j++)
			state &= result.source[i][j].state;

		hse::iterator split_t = result.create(hse::transition(hse::transition::active, state));
		result.connect(sm, split_t);

		for (int j = 0; j < (int)result.source[i].size(); j++)
			result.connect(split_t, hse::iterator(hse::place::type, result.source[i][j].index));
	}

	for (int i = 0; i < (int)result.sink.size(); i++)
	{
		hse::iterator merge_t = result.create(hse::transition());
		result.connect(merge_t, sm);

		for (int j = 0; j < (int)result.sink[i].size(); j++)
			result.connect(hse::iterator(hse::place::type, result.sink[i][j].index), merge_t);
	}

	hse::iterator guard = result.create(hse::transition(hse::transition::passive, ~repeat));
	result.connect(sm, guard);
	hse::iterator arrow = result.create(hse::place());
	result.connect(guard, arrow);

	result.source.clear();
	result.source.push_back(vector<hse::local_token>(1, hse::local_token(sm.index, boolean::cube())));
	result.sink.clear();
	result.sink.push_back(vector<hse::local_token>(1, hse::local_token(arrow.index, boolean::cube())));

	return result;
}
