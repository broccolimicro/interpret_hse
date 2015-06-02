/*
 * export.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "export.h"

/*parse_hse::sequence export_sequence(vector<hse::iterator> &i, const hse::graph &g, boolean::variable_set &v)
{
	parse_hse::sequence result;
	result.valid = true;

	vector<hse::iterator> covered;

	while (1)
	{
		if (i.size() == 1 && i[0].type == hse::transition::type && g.transitions[i[0].index].behavior == hse::transition::active)
			result.actions.push_back(new parse_boolean::internal_choice(export_internal_choice(g.transitions[i[0].index].action, v)));
		else if (i.size() == 1 && i[0].type == hse::transition::type && g.transitions[i[0].index].behavior == hse::transition::passive)
		{
			parse_hse::condition *c = new parse_hse::condition();
			c->valid = true;
			c->branches.resize(1);
			c->branches[0].first = export_disjunction(g.transitions[i[0].index].action, v);
			result.actions.push_back(c);
		}
		else if (i.size() > 1 && i[0].type == hse::place::type)
			result.actions.push_back(new parse_hse::parallel(export_parallel(i, g, v)));
		else if (i.size() > 1 && i[0].type == hse::transition::type)
			result.actions.push_back(new parse_hse::condition(export_condition(i, g, v)));

		vector<hse::iterator> n = g.next(i);
		sort(n.begin(), n.end());
		n.resize(unique(n.begin(), n.end()) - n.begin());

		vector<hse::iterator> p = g.prev(n);
		sort(p.begin(), p.end());
		p.resize(unique(p.begin(), p.end()) - p.begin());


		if (vector_intersection_size(covered, p) != 0 || p.size() > i.size())
			return result;
		else
		{
			covered.insert(covered.end(), i.begin(), i.end());
			i = n;
		}
	}
}

parse_hse::parallel export_parallel(vector<hse::iterator> &i, const hse::graph &g, boolean::variable_set &v)
{
	parse_hse::parallel result;
	result.valid = true;
	vector<hse::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<hse::iterator> start(1, i[j]);
		result.branches = export_sequence(start, g, v);
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}

parse::syntax *export_condition(vector<hse::iterator> &i, const hse::graph &g, boolean::variable_set &v)
{
	parse_hse::condition result;
	result.valid = true;
	vector<hse::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<hse::iterator> start(1, i[j]);
		result.branches.push_back(pair<parse_boolean::disjunction, parse_hse::parallel>());
		result.branches.back().second.valid = true;
		parse_hse::sequence s = export_sequence(start, g, v);
		if (s.actions.size() > 0 && s.actions[0]->is_a<parse_hse::condition>() &&
			((parse_hse::condition*)s.actions[0])->branches.size() == 1 &&
			((parse_hse::condition*)s.actions[0])->branches.back().second.branches.size() == 0)
		{
			result.branches.back().first = ((parse_hse::condition*)s.actions[0])->branches.back().first;
			delete s.actions[0];
			s.actions.erase(s.actions.begin());
		}
		else
			result.branches.back().first = export_disjunction(boolean::cover(boolean::cube()), v);

		result.branches.back().second.branches.push_back(s);

		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}*/


parse_hse::sequence export_sequence(vector<hse::iterator> nodes, map<hse::iterator, int> counts, const hse::graph &g, boolean::variable_set &v)
{
	// Maintain a stack to help us manage the hierarchy. The deeper
	// the stack, the more hierarchy there is. This stack stores
	// only sequences. Since we know at this point that every parallel
	// or conditional block we introduce will only have one branch, we
	// can get away with this. We will merge these sequences later to
	// add parallelism or choice.
	parse_hse::sequence head;
	head.valid = true;
	vector<parse_hse::sequence*> stack;
	stack.push_back(&head);

	int delta = 0;
	int value = counts[nodes[0]]-1;
	hse::iterator last = nodes[0];
	for (int j = 1; j < (int)nodes.size(); j++)
	{
		int c = counts[nodes[j]]-1;
		delta = c - value;
		value = c;

		// The count increased, we need to remove hierarchy
		if (delta > 0 && j > 1)
		{
			if (stack.size() == 0)
			{
				error("", "hse not properly nested", __FILE__, __LINE__);
				return parse_hse::sequence();
			}

			stack.back()->end = last.index;

			if (stack.size() > 1)
				stack[stack.size()-2]->actions.back()->end = nodes[j].index;

			stack.pop_back();
			delta--;
		}

		// The count decreased, we need to add hierarchy
		if (delta < 0)
		{
			// The last node before the count decrease was a transition, meaning
			// we need to wrap the next couple transitions in a parallel block
			if (last.type == hse::transition::type)
			{
				parse_hse::parallel *tmp = new parse_hse::parallel();
				tmp->valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp->start = last.index;

				parse_hse::sequence new_head;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp->branches.push_back(new_head);

				stack.back()->actions.push_back(tmp);
				stack.push_back(&tmp->branches.back());
			}
			// The last node before the count decrease was a place, meaning we need
			// to wrap the next couple transitions in a conditional block. Never
			// mind the guard, we will take care of that later.
			else if (last.type == hse::place::type)
			{
				parse_hse::condition *tmp = new parse_hse::condition();
				tmp->valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp->start = last.index;

				tmp->branches.push_back(pair<parse_boolean::guard, parse_hse::parallel>());
				tmp->branches.back().second.valid = true;
				parse_hse::sequence new_head;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp->branches.back().second.branches.push_back(new_head);

				stack.back()->actions.push_back(tmp);
				stack.push_back(&tmp->branches.back().second.branches.back());
			}

			delta++;
		}

		// Once we have dealt with the hierarchy issues, we still need to add assignments and disjunctions into the hse block.
		// We'll package the disjunctions into conditionals later.
		if (stack.size() == 0)
		{
			internal("", "empty stack", __FILE__, __LINE__);
			return head;
		}
		else if (nodes[j].type == hse::transition::type && g.transitions[nodes[j].index].behavior == hse::transition::active)
		{
			stack.back()->actions.push_back(new parse_boolean::assignment(export_assignment(g.transitions[nodes[j].index].action, v)));
			stack.back()->actions.back()->start = nodes[j].index;
			stack.back()->actions.back()->end = nodes[j].index;
		}
		else if (nodes[j].type == hse::transition::type && g.transitions[nodes[j].index].behavior == hse::transition::passive)
		{
			stack.back()->actions.push_back(new parse_boolean::guard(export_guard_xfactor(g.transitions[nodes[j].index].action, v)));
			stack.back()->actions.back()->start = nodes[j].index;
			stack.back()->actions.back()->end = nodes[j].index;
		}

		last = nodes[j];
	}

	return head;
}


// TODO this doesn't handle the case where one sequence is a subset of another.
bool merge_sequences(parse_hse::sequence &s0, parse_hse::sequence &s1, vector<parse::syntax*> &m)
{
	int offset = 0;
	bool equal = false;
	if (s0.actions.size() >= s1.actions.size())
	{
		for (offset = 0; offset < (int)s0.actions.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s1.actions.size() && equal; k++)
				if (s0.actions[(offset + k)%s0.actions.size()]->start != s1.actions[k]->start)
					equal = false;
		}
	}
	else if (s1.actions.size() > s0.actions.size())
	{
		for (offset = 0; offset < (int)s1.actions.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s0.actions.size() && equal; k++)
				if (s1.actions[(offset + k)%s1.actions.size()]->start != s0.actions[k]->start)
					equal = false;
		}

		if (equal)
			swap(s0, s1);
	}

	offset--;

	for (int k = 0; k < (int)s1.actions.size() && equal; k++)
	{
		if (s0.actions[(offset+k)%s0.actions.size()]->is_a<parse_hse::parallel>() &&
			s1.actions[k]->is_a<parse_hse::parallel>())
		{
			parse_hse::parallel *tmp0 = (parse_hse::parallel *)s0.actions[(offset+k)%s0.actions.size()];
			parse_hse::parallel *tmp1 = (parse_hse::parallel *)s1.actions[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
		else if (s0.actions[(offset+k)%s0.actions.size()]->is_a<parse_hse::condition>() &&
				s1.actions[k]->is_a<parse_hse::condition>())
		{
			parse_hse::condition *tmp0 = (parse_hse::condition *)s0.actions[(offset+k)%s0.actions.size()];
			parse_hse::condition *tmp1 = (parse_hse::condition *)s1.actions[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
	}

	return equal;
}

// TODO this doesn't know how to handle non-deterministic choice yet.
// The result is that all non-deterministic conditionals are converted to deterministic ones.
// TODO This function does not always throw an error when an HSE is not properly nested.
// It will just return a bunch of independent processes that don't correctly implement the HSE.
parse_hse::parallel export_parallel(const hse::graph &g, boolean::variable_set &v)
{
	// First step is to identify all of the cycles in the graph.
	vector<vector<hse::iterator> > cycles = g.cycles();

	/*for (int i = 0; i < (int)cycles.size(); i++)
	{
		printf("{");
		for (int j = 0; j < (int)cycles[i].size(); j++)
		{
			if (j != 0)
				printf(", ");
			printf("%c%d", cycles[i][j].type == hse::place::type ? 'P' : 'T', cycles[i][j].index);
		}
		printf("}\n");
	}*/

	// We then need to figure out where add hierarchy. We can do this by looking at the
	// number of cycles that share a given node. If that number decreases, then we need
	// to increase the hierarchy level. If that number increases, then we need to decrease
	// the hierarchy level. Knowing this, we can just iterate over all the nodes in each
	// cycle and add either a condition or a parallel depending upon the type of the last
	// node we passed.
	map<hse::iterator, int> counts;
	for (int i = 0; i < (int)cycles.size(); i++)
		count_elements(cycles[i], counts);

	parse_hse::parallel wrapper;
	wrapper.valid = true;
	for (int i = 0; i < (int)cycles.size(); i++)
	{
		// Put all of the generated sequences into one parallel block. Remember that
		// these sequences still need to be wrapped in a loop, but we'll do that in a later step.
		wrapper.branches.push_back(export_sequence(cycles[i], counts, g, v));
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Our next step is to merge corresponding sequences in this wrapper.
	// If the HSE is properly nested then we can merge two sequences if
	// one is a subset or equals another. When we run up against a conditional
	// or parallel block that exists in both, we merge them, and we can just
	// ignore everything else. This process needs to happen recursively.
	// (though the data recursion is not implemented as a function recursion)
	vector<parse::syntax*> m;
	m.push_back(&wrapper);

	while (m.size() > 0)
	{
		sort(m.begin(), m.end());
		m.resize(unique(m.begin(), m.end()) - m.begin());
		parse::syntax *syn = m.back();
		m.pop_back();

		if (syn->is_a<parse_hse::parallel>())
		{
			parse_hse::parallel *par = (parse_hse::parallel *)syn;

			for (int i = 0; i < (int)par->branches.size(); i++)
				for (int j = (int)par->branches.size()-1; j >= i+1; j--)
					if (merge_sequences(par->branches[i], par->branches[j], m))
						par->branches.erase(par->branches.begin() + j);
		}
		else if (syn->is_a<parse_hse::condition>())
		{
			parse_hse::condition *par = (parse_hse::condition *)syn;

			for (int i = 0; i < (int)par->branches.size(); i++)
				for (int j = (int)par->branches.size()-1; j >= i+1; j--)
					if (merge_sequences(par->branches[i].second.branches[0], par->branches[j].second.branches[0], m))
						par->branches.erase(par->branches.begin() + j);
		}
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Next, we need to fix the syntax for all of the conditionals by finding their appropriate disjunctions
	// and moving them to the right slots. We also wrap guards in their own conditional here. This process
	// requires recursion (again, not functionally implemented).
	m.push_back(&wrapper);
	while(m.size() > 0)
	{
		parse::syntax *syn = m.back();
		m.pop_back();

		// We've run up against a conditional
		if (syn->is_a<parse_hse::condition>())
		{
			parse_hse::condition *c = (parse_hse::condition*)syn;
			for (int i = 0; i < (int)c->branches.size(); i++)
			{
				// Check to see if the first item in the sequence of this branch is a disjunction
				if (c->branches[i].second.branches.size() == 1 && c->branches[i].second.branches[0].actions.size() > 0 && c->branches[i].second.branches[0].actions[0]->is_a<parse_boolean::guard>())
				{
					// if it is, we can move it to the guard
					c->branches[i].first = *(parse_boolean::guard*)c->branches[i].second.branches[0].actions[0];
					delete c->branches[i].second.branches[0].actions[0];
					c->branches[i].second.branches[0].actions.erase(c->branches[i].second.branches[0].actions.begin());
				}
				else
					// if it isn't then we need to write our own guard.
					c->branches[i].first = export_guard(boolean::cube(), v);

				// recurse
				m.push_back(&c->branches[i].second);
			}
		}
		// parallel block, we just recurse here
		else if (syn->is_a<parse_hse::parallel>())
		{
			parse_hse::parallel *p = (parse_hse::parallel*)syn;
			for (int i = 0; i < (int)p->branches.size(); i++)
				m.push_back(&p->branches[i]);
		}
		// A sequence, find all of the disjunctions that don't belong to conditionals
		// and give them their own conditional. Then, recurse.
		else if (syn->is_a<parse_hse::sequence>())
		{
			parse_hse::sequence *s = (parse_hse::sequence*)syn;
			for (int i = 0; i < (int)s->actions.size(); i++)
			{
				if (s->actions[i]->is_a<parse_boolean::guard>())
				{
					parse_hse::condition *c = new parse_hse::condition();
					c->valid = true;
					c->deterministic = true;
					c->branches.resize(1);
					c->branches.back().first = *((parse_boolean::guard*)s->actions[i]);
					c->start = c->branches.back().first.start;
					delete s->actions[i];
					s->actions[i] = c;
				}
				else
					m.push_back(s->actions[i]);
			}
		}
	}

	//printf("%s\n", wrapper.to_string().c_str());

	// Finally, just wrap all of the sequences in infinite loops and call it a day.
	for (int i = 0; i < (int)wrapper.branches.size(); i++)
	{
		parse_hse::loop *l = new parse_hse::loop();
		l->valid = true;
		l->branches.resize(1);
		l->branches[0].second.branches.resize(1);
		l->branches[0].second.branches[0] = wrapper.branches[i];
		wrapper.branches[i].clear();
		wrapper.branches[i].actions.push_back(l);
	}

	//printf("%s\n", wrapper.to_string().c_str());

	return wrapper;
}
