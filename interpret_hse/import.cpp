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
	result.source.push_back(result.create(hse::transition(hse::transition::passive, import_cover(tokens, syntax, variables, auto_define))));
	result.sink = result.source;
	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_boolean::internal_choice &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;
	result.source.push_back(result.create(hse::transition(hse::transition::active, import_cover(tokens, syntax, variables, auto_define))));
	result.sink = result.source;
	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::sequence &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;
	for (int i = 0; i < (int)syntax.actions.size(); i++)
		if (syntax.actions[i] != NULL && syntax.actions[i]->valid)
		{
			if (syntax.actions[i]->is_a<parse_hse::parallel>())
				result.sequence(import_graph(tokens, *(parse_hse::parallel*)syntax.actions[i], variables, auto_define));
			else if (syntax.actions[i]->is_a<parse_hse::condition>())
				result.sequence(import_graph(tokens, *(parse_hse::condition*)syntax.actions[i], variables, auto_define));
			else if (syntax.actions[i]->is_a<parse_hse::loop>())
				result.sequence(import_graph(tokens, *(parse_hse::loop*)syntax.actions[i], variables, auto_define));
			else if (syntax.actions[i]->is_a<parse_boolean::internal_choice>())
				result.sequence(import_graph(tokens, *(parse_boolean::internal_choice*)syntax.actions[i], variables, auto_define));
		}

	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::parallel &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	hse::iterator split = result.create(hse::transition());

	for (int i = 0; i < (int)syntax.branches.size(); i++)
		if (syntax.branches[i].valid)
			result.merge(import_graph(tokens, syntax.branches[i], variables, auto_define));

	hse::iterator merge = result.create(hse::transition());

	result.connect(split, result.source);
	result.connect(result.sink, merge);
	result.source.clear();
	result.sink.clear();
	result.source.push_back(split);
	result.sink.push_back(merge);

	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::condition &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	hse::iterator split = result.create(hse::place());

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid)
			branch.sequence(import_graph(tokens, syntax.branches[i].first, variables, auto_define));
		if (syntax.branches[i].second.valid)
			branch.sequence(import_graph(tokens, syntax.branches[i].second, variables, auto_define));
		result.merge(branch);
	}

	hse::iterator merge = result.create(hse::place());

	result.connect(split, result.source);
	result.connect(result.sink, merge);
	result.source.clear();
	result.sink.clear();
	result.source.push_back(split);
	result.sink.push_back(merge);

	return result;
}

hse::graph import_graph(tokenizer &tokens, const parse_hse::loop &syntax, boolean::variable_set &variables, bool auto_define)
{
	hse::graph result;

	hse::iterator split = result.create(hse::place());

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid)
			branch.sequence(import_graph(tokens, syntax.branches[i].first, variables, auto_define));
		if (syntax.branches[i].second.valid)
			branch.sequence(import_graph(tokens, syntax.branches[i].second, variables, auto_define));
		result.merge(branch);
	}

	boolean::cover repeat = 0;
	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		if (syntax.branches[i].first.valid)
			repeat |= import_cover(tokens, syntax.branches[i].first, variables, auto_define);
		else
			repeat = 1;
	}

	hse::iterator guard = result.create(hse::transition(hse::transition::passive, ~repeat));
	result.connect(split, guard);

	result.connect(split, result.source);
	result.connect(result.sink, split);
	result.source.clear();
	result.sink.clear();
	result.source.push_back(split);
	result.sink.push_back(guard);

	return result;
}
