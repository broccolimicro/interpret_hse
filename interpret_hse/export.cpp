/*
 * export.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "export.h"

parse_hse::sequence export_sequence(hse::iterator &branch, const hse::graph &g, boolean::variable_set &v)
{
	parse_hse::sequence result;
/*	result.valid = true;

	vector<hse::iterator> c(1, branch), n, p;

	do
	{
		n = g.next(c);
		p = g.prev(n);

		sort(n.begin(), n.end());
		sort(p.begin(), p.end());
		n.resize(unique(n.begin(), n.end()) - n.begin());
		p.resize(unique(p.begin(), p.end()) - p.begin());

		if (c.size() == 1)
		{
			if (c[0].type == hse::transition::type)
			{
				if (g.transitions[c[0].index].behavior == hse::transition::active)
					result.actions.push_back(new parse_boolean::internal_choice(export_internal_choice(g.transitions[c[0].index].action, v)));
				else
					result.actions.push_back(new parse_boolean::disjunction(export_disjunction(g.transitions[c[0].index].action, v)));
			}
		}
		else
		{

		}

		c = n;
	} while (n.size() > 0 && p.size() <= 1);
*/
	return result;
}

parse_hse::condition export_condition(vector<hse::iterator> &branches, const hse::graph &g, boolean::variable_set &v)
{
	parse_hse::condition result;
	/*result.valid = true;
	for (int i = 0; i < (int)branches.size(); i++)
	{
		if (branches[i].type == hse::place::type)
			internal("", "conditional branch must start with a transition", __FILE__, __LINE__);
		else
		{
			parse_hse::sequence s = export_sequence(branches[i], g, v);
			if (s.actions.size() > 0 && s.actions[0]->is_a<parse_boolean::disjunction>())
			{
				parse_boolean::disjunction d = *((parse_boolean::disjunction*)s.actions[0]);
				delete s.actions[0];
				s.actions.erase(s.actions.begin());
				result.branches.push_back(pair<parse_boolean::disjunction, parse_hse::sequence>(d, s));
			}
			else
				result.branches.push_back(pair<parse_boolean::disjunction, parse_hse::sequence>(export_disjunction(boolean::cover(boolean::cube()), v), s));
		}
	}
*/
	return result;
}

parse_hse::parallel export_parallel(vector<hse::iterator> &branches, const hse::graph &g, boolean::variable_set &v)
{
	parse_hse::parallel result;
	/*result.valid = true;
	for (int i = 0; i < (int)branches.size(); i++)
		result.branches.push_back(export_sequence(branches[i], g, v));*/
	return result;
}
