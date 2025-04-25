#include "export_chp.h"
#include "export_expr.h"

#include <interpret_boolean/export.h>

namespace hse {

parse_chp::composition export_sequence(vector<petri::iterator> &i, const hse::graph &g)
{
	parse_chp::composition result;
	result.valid = true;
	result.level = 1;

	vector<petri::iterator> covered;

	while (1)
	{
		if (i.size() == 1 && i[0].type == hse::transition::type)
		{
			if (not g.transitions[i[0].index].guard.is_tautology()) {
				parse_chp::control c;
				c.valid = true;
				c.branches.resize(1);
				c.branches[0].first = boolean::export_expression(g.transitions[i[0].index].guard, g);
				result.branches.push_back(parse_chp::branch(c));
			}

			if (g.transitions[i[0].index].local_action.cubes.size() == 1)
			{
				vector<int> vars = g.transitions[i[0].index].local_action.cubes[0].vars();
				if (vars.size() == 1)
					result.branches.push_back(parse_chp::branch(boolean::export_assignment(vars[0], g.transitions[i[0].index].local_action.cubes[0].get(vars[0]), g)));
				else
					result.branches.push_back(parse_chp::branch(export_parallel(g.transitions[i[0].index].local_action.cubes[0], g)));
			}
			else
				result.branches.push_back(parse_chp::branch(export_control(g.transitions[i[0].index].local_action, g)));
		}
		else if (i.size() > 1 && i[0].type == hse::place::type)
			result.branches.push_back(parse_chp::branch(export_parallel(i, g)));

		vector<petri::iterator> n = g.next(i);
		sort(n.begin(), n.end());
		n.resize(unique(n.begin(), n.end()) - n.begin());

		vector<petri::iterator> p = g.prev(n);
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

parse_chp::composition export_parallel(vector<petri::iterator> &i, const hse::graph &g)
{
	parse_chp::composition result;
	result.valid = true;
	result.level = 0;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(parse_chp::branch(export_sequence(start, g)));
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}

parse_chp::control export_control(vector<petri::iterator> &i, const hse::graph &g)
{
	parse_chp::control result;
	result.valid = true;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(pair<parse_expression::expression, parse_chp::composition>());
		result.branches.back().second.valid = true;
		parse_chp::composition s = export_sequence(start, g);
		if (s.branches.size() > 0 && s.branches[0].ctrl.valid && s.branches[0].ctrl.branches.size() == 1 &&
			s.branches[0].ctrl.branches.back().second.branches.size() == 0)
		{
			result.branches.back().first = s.branches[0].ctrl.branches.back().first;
			s.branches.erase(s.branches.begin());
		}
		else
			result.branches.back().first = boolean::export_expression(boolean::cover(boolean::cube()), g);

		result.branches.back().second.branches.push_back(s);

		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}


/*parse_chp::composition export_sequence(vector<petri::iterator> nodes, map<petri::iterator, int> counts, const hse::graph &g, const ucs::variable_set &v)
{
	// Maintain a stack to help us manage the hierarchy. The deeper
	// the stack, the more hierarchy there is. This stack stores
	// only sequences. Since we know at this point that every parallel
	// or conditional block we introduce will only have one branch, we
	// can get away with this. We will merge these sequences later to
	// add parallelism or choice.
	parse_chp::composition head;
	head.valid = true;
	head.level = 1;
	vector<parse_chp::composition*> stack;
	stack.push_back(&head);

	int delta = 0;
	int value = counts[nodes[0]]-1;
	petri::iterator last = nodes[0];
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
				return parse_chp::composition();
			}

			stack.back()->end = last.index;

			if (stack.size() > 1)
			{
				stack[stack.size()-2]->branches.back().assign.end = nodes[j].index;
				stack[stack.size()-2]->branches.back().ctrl.end = nodes[j].index;
				stack[stack.size()-2]->branches.back().sub.end = nodes[j].index;
			}

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
				parse_chp::composition tmp;
				tmp.valid = true;
				tmp.level = 0;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp.start = last.index;

				parse_chp::composition new_head;
				new_head.valid = true;
				new_head.level = 1;
				new_head.start = nodes[j].index;
				tmp.branches.push_back(parse_chp::branch(new_head));

				stack.back()->branches.push_back(parse_chp::branch(tmp));
				stack.push_back(&stack.back()->branches.back().sub.branches.back().sub);
			}
			// The last node before the count decrease was a place, meaning we need
			// to wrap the next couple transitions in a conditional block. Never
			// mind the guard, we will take care of that later.
			else if (last.type == hse::place::type)
			{
				parse_chp::control tmp;
				tmp.valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp.start = last.index;

				tmp.branches.back().second.valid = true;
				parse_chp::composition new_head;
				new_head.level = 1;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp.branches.push_back(pair<parse_expression::expression, parse_chp::composition>(parse_expression::expression(), new_head));

				stack.back()->branches.push_back(parse_chp::branch(tmp));
				stack.push_back(&stack.back()->branches.back().ctrl.branches.back().second);
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
			if (g.transitions[nodes[j].index].local_action.cubes.size() == 1)
			{
				vector<int> vars = g.transitions[nodes[j].index].local_action.cubes[0].vars();
				if (vars.size() == 1)
					stack.back()->branches.push_back(parse_chp::branch(export_assignment(vars[0], g.transitions[nodes[j].index].local_action.cubes[0].get(vars[0]), v)));
				else
					stack.back()->branches.push_back(parse_chp::branch(boolean::export_composition(g.transitions[nodes[j].index].local_action, v)));
			}
			else
				stack.back()->branches.push_back(parse_chp::branch(export_control(g.transitions[nodes[j].index].local_action, v)));

			stack.back()->branches.back().ctrl.start = nodes[j].index;
			stack.back()->branches.back().ctrl.end = nodes[j].index;
			stack.back()->branches.back().sub.start = nodes[j].index;
			stack.back()->branches.back().sub.end = nodes[j].index;
			stack.back()->branches.back().assign.start = nodes[j].index;
			stack.back()->branches.back().assign.end = nodes[j].index;
		}
		else if (nodes[j].type == hse::transition::type && g.transitions[nodes[j].index].behavior == hse::transition::passive)
		{
			stack.back()->branches.push_back(parse_chp::branch(parse_chp::control()));
			stack.back()->branches.back().ctrl.branches.push_back(pair<parse_expression::expression, parse_chp::composition>(boolean::export_expression_xfactor(g.transitions[nodes[j].index].local_action, v), parse_chp::composition()));

			stack.back()->branches.back().ctrl.start = nodes[j].index;
			stack.back()->branches.back().ctrl.end = nodes[j].index;
			stack.back()->branches.back().sub.start = nodes[j].index;
			stack.back()->branches.back().sub.end = nodes[j].index;
			stack.back()->branches.back().assign.start = nodes[j].index;
			stack.back()->branches.back().assign.end = nodes[j].index;
		}

		last = nodes[j];
	}

	return head;
}

// TODO this doesn't handle the case where one sequence is a subset of another.
bool merge_sequences(parse_chp::composition &s0, parse_chp::composition &s1, vector<parse::syntax*> &m)
{
	int offset = 0;
	bool equal = false;
	if (s0.branches.size() >= s1.branches.size())
	{
		for (offset = 0; offset < (int)s0.branches.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s1.branches.size() && equal; k++)
				if (s0.branches[(offset + k)%s0.branches.size()].sub.start != s1.branches[k].sub.start)
					equal = false;
		}
	}
	else if (s1.branches.size() > s0.branches.size())
	{
		for (offset = 0; offset < (int)s1.branches.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s0.branches.size() && equal; k++)
				if (s1.branches[(offset + k)%s1.branches.size()].sub.start != s0.branches[k].sub.start)
					equal = false;
		}

		if (equal)
			swap(s0, s1);
	}

	offset--;

	for (int k = 0; k < (int)s1.branches.size() && equal; k++)
	{
		if (s0.branches[(offset+k)%s0.branches.size()]->is_a<parse_hse::parallel>() &&
			s1.branches[k]->is_a<parse_hse::parallel>())
		{
			parse_hse::parallel *tmp0 = (parse_hse::parallel *)s0.branches[(offset+k)%s0.branches.size()];
			parse_hse::parallel *tmp1 = (parse_hse::parallel *)s1.branches[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
		else if (s0.branches[(offset+k)%s0.branches.size()]->is_a<parse_hse::condition>() &&
				s1.branches[k]->is_a<parse_hse::condition>())
		{
			parse_hse::condition *tmp0 = (parse_hse::condition *)s0.branches[(offset+k)%s0.branches.size()];
			parse_hse::condition *tmp1 = (parse_hse::condition *)s1.branches[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
	}

	return equal;
}*/

}
