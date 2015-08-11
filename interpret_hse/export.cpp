/*
 * export.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "export.h"

parse_dot::node_id export_node_id(const petri::iterator &i)
{
	parse_dot::node_id result;
	result.valid = true;
	result.id = (i.type == hse::transition::type ? "T" : "P") + ::to_string(i.index);
	return result;
}

parse_dot::attribute_list export_attribute_list(const hse::iterator i, const hse::graph &g, ucs::variable_set &variables, bool labels)
{
	parse_dot::attribute_list result;
	result.valid = true;
	parse_dot::assignment_list sub_result;
	sub_result.valid = true;
	parse_dot::assignment label;
	label.valid = true;
	label.first = "label";
	parse_dot::assignment circle;
	circle.valid = true;
	circle.first = "shape";
	circle.second = "ellipse";
	parse_dot::assignment plaintext;
	plaintext.valid = true;
	plaintext.first = "shape";
	plaintext.second = "plaintext";
	parse_dot::assignment marked;
	marked.valid = true;
	marked.first = "style";
	marked.second = "filled";

	if (i.type == hse::place::type)
	{
		sub_result.as.push_back(circle);
		for (int j = 0; j < (int)g.source.size(); j++)
			for (int k = 0; k < (int)g.source[j].tokens.size(); k++)
				if (i.index == g.source[j].tokens[k].index)
					sub_result.as.push_back(marked);

		label.second = export_expression_hfactor(g.places[i.index].effective, variables).to_string();
	}
	else
	{
		sub_result.as.push_back(plaintext);
		if (g.transitions[i.index].behavior == hse::transition::active)
			label.second = export_composition(g.transitions[i.index].local_action, variables).to_string();
		if (g.transitions[i.index].behavior == hse::transition::passive)
			label.second = "[ " + export_expression_xfactor(g.transitions[i.index].local_action, variables).to_string() + " ]";
	}

	if (labels)
		label.second = (i.type == hse::place::type ? "P" : "T") + to_string(i.index) + ":" + label.second;

	sub_result.as.push_back(label);

	result.attributes.push_back(sub_result);
	return result;
}

parse_dot::statement export_statement(const hse::iterator &i, const hse::graph &g, ucs::variable_set &v, bool labels)
{
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "node";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(i)));
	result.attributes = export_attribute_list(i, g, v, labels);
	return result;
}

parse_dot::statement export_statement(const pair<int, int> &a, const hse::graph &g, ucs::variable_set &v, bool labels)
{
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "edge";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(g.arcs[a.first][a.second].from)));
	result.nodes.push_back(new parse_dot::node_id(export_node_id(g.arcs[a.first][a.second].to)));
	parse_dot::assignment_list attr;
	attr.valid = true;
	parse_dot::assignment label;
	label.valid = true;
	label.first = "label";
	label.second = "A" + to_string(a.first) + "." + to_string(a.second);
	attr.as.push_back(label);
	if (labels)
	{
		result.attributes.valid = true;
		result.attributes.attributes.push_back(attr);
	}

	return result;
}

parse_dot::statement export_statement(const hse::half_synchronization &s)
{
	parse_dot::statement result;
	result.valid = true;
	result.statement_type = "edge";
	result.nodes.push_back(new parse_dot::node_id(export_node_id(hse::iterator(hse::transition::type, s.active.index))));
	result.nodes.push_back(new parse_dot::node_id(export_node_id(hse::iterator(hse::transition::type, s.passive.index))));

	parse_dot::assignment_list attr;
	attr.valid = true;
	parse_dot::assignment dotted;
	dotted.valid = true;
	dotted.first = "style";
	dotted.second = "dashed";
	parse_dot::assignment constraint;
	constraint.valid = true;
	constraint.first = "constraint";
	constraint.second = "false";
	attr.as.push_back(dotted);
	attr.as.push_back(constraint);
	result.attributes.attributes.push_back(attr);
	return result;
}

parse_dot::graph export_graph(const hse::graph &g, ucs::variable_set &v, bool labels)
{
	parse_dot::graph result;
	result.valid = true;
	result.id = "hse";
	result.type = "digraph";

	for (int i = 0; i < (int)g.places.size(); i++)
		result.statements.push_back(export_statement(hse::iterator(hse::place::type, i), g, v, labels));

	for (int i = 0; i < (int)g.transitions.size(); i++)
		result.statements.push_back(export_statement(hse::iterator(hse::transition::type, i), g, v, labels));

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)g.arcs[i].size(); j++)
			result.statements.push_back(export_statement(pair<int, int>(i, j), g, v, labels));

	for (int i = 0; i < (int)g.synchronizations.size(); i++)
		result.statements.push_back(export_statement(g.synchronizations[i]));

	return result;
}

parse_chp::composition export_composition(boolean::cube c, ucs::variable_set &variables)
{
	parse_chp::composition result;
	result.valid = true;

	result.level = 2;//parse_expression::composition::get_level(",");

	for (int i = 0; i < (int)variables.nodes.size(); i++)
		if (c.get(i) != 2)
			result.branches.push_back(export_assignment(i, c.get(i), variables));

	return result;
}

parse_chp::control export_control(boolean::cover c, ucs::variable_set &variables)
{
	parse_chp::control result;
	result.valid = true;

	result.deterministic = false;

	for (int i = 0; i < (int)c.cubes.size(); i++)
		result.branches.push_back(pair<parse_expression::expression, parse_chp::composition>(parse_expression::expression(), export_composition(c.cubes[i], variables)));

	return result;
}


parse_chp::composition export_sequence(vector<petri::iterator> &i, const hse::graph &g, ucs::variable_set &v)
{
	parse_chp::composition result;
	result.valid = true;
	result.level = 1;

	vector<petri::iterator> covered;

	while (1)
	{
		if (i.size() == 1 && i[0].type == hse::transition::type && g.transitions[i[0].index].behavior == hse::transition::active)
		{
			if (g.transitions[i[0].index].local_action.cubes.size() == 1)
			{
				vector<int> vars = g.transitions[i[0].index].local_action.cubes[0].vars();
				if (vars.size() == 1)
					result.branches.push_back(parse_chp::branch(export_assignment(vars[0], g.transitions[i[0].index].local_action.cubes[0].get(vars[0]), v)));
				else
					result.branches.push_back(parse_chp::branch(export_composition(g.transitions[i[0].index].local_action.cubes[0], v)));
			}
			else
				result.branches.push_back(parse_chp::branch(export_control(g.transitions[i[0].index].local_action, v)));
		}
		else if (i.size() == 1 && i[0].type == hse::transition::type && g.transitions[i[0].index].behavior == hse::transition::passive)
		{
			parse_chp::control c;
			c.valid = true;
			c.branches.resize(1);
			c.branches[0].first = export_expression(g.transitions[i[0].index].local_action, v);
			result.branches.push_back(parse_chp::branch(c));
		}
		else if (i.size() > 1 && i[0].type == hse::place::type)
			result.branches.push_back(parse_chp::branch(export_parallel(i, g, v)));
		else if (i.size() > 1 && i[0].type == hse::transition::type)
			result.branches.push_back(parse_chp::branch(export_control(i, g, v)));

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

parse_chp::composition export_parallel(vector<petri::iterator> &i, const hse::graph &g, ucs::variable_set &v)
{
	parse_chp::composition result;
	result.valid = true;
	result.level = 0;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(parse_chp::branch(export_sequence(start, g, v)));
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}

parse_chp::control export_control(vector<petri::iterator> &i, const hse::graph &g, ucs::variable_set &v)
{
	parse_chp::control result;
	result.valid = true;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(pair<parse_expression::expression, parse_chp::composition>());
		result.branches.back().second.valid = true;
		parse_chp::composition s = export_sequence(start, g, v);
		if (s.branches.size() > 0 && s.branches[0].ctrl.valid && s.branches[0].ctrl.branches.size() == 1 &&
			s.branches[0].ctrl.branches.back().second.branches.size() == 0)
		{
			result.branches.back().first = s.branches[0].ctrl.branches.back().first;
			s.branches.erase(s.branches.begin());
		}
		else
			result.branches.back().first = export_expression(boolean::cover(boolean::cube()), v);

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
					stack.back()->branches.push_back(parse_chp::branch(export_composition(g.transitions[nodes[j].index].local_action, v)));
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
			stack.back()->branches.back().ctrl.branches.push_back(pair<parse_expression::expression, parse_chp::composition>(export_expression_xfactor(g.transitions[nodes[j].index].local_action, v), parse_chp::composition()));

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
}

// TODO this doesn't know how to handle non-deterministic choice yet.
// The result is that all non-deterministic conditionals are converted to deterministic ones.
// TODO This function does not always throw an error when an HSE is not properly nested.
// It will just return a bunch of independent processes that don't correctly implement the HSE.
parse_hse::parallel export_parallel(const hse::graph &g, const boolean::variable_set &v)
{
	// First step is to identify all of the cycles in the graph.
	vector<vector<petri::iterator> > cycles = g.cycles();

	for (int i = 0; i < (int)cycles.size(); i++)
	{
		printf("{");
		for (int j = 0; j < (int)cycles[i].size(); j++)
		{
			if (j != 0)
				printf(", ");
			printf("%c%d", cycles[i][j].type == hse::place::type ? 'P' : 'T', cycles[i][j].index);
		}
		printf("}\n");
	}

	// We then need to figure out where add hierarchy. We can do this by looking at the
	// number of cycles that share a given node. If that number decreases, then we need
	// to increase the hierarchy level. If that number increases, then we need to decrease
	// the hierarchy level. Knowing this, we can just iterate over all the nodes in each
	// cycle and add either a condition or a parallel depending upon the type of the last
	// node we passed.
	map<petri::iterator, int> counts;
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
					c->branches[i].first = export_expression(boolean::cube(), v);

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
}*/

string export_node(petri::iterator i, const hse::graph &g, const ucs::variable_set &v)
{
	vector<petri::iterator> n = g.next(i);
	vector<petri::iterator> p = g.prev(i);
	string result = "";

	if (i.type == hse::transition::type)
	{
		vector<petri::iterator> pp;
		vector<petri::iterator> np;

		//bool proper_nest = true;
		for (int j = 0; j < (int)p.size(); j++)
		{
			vector<petri::iterator> tmp = g.prev(p[j]);
			pp.insert(pp.begin(), tmp.begin(), tmp.end());
			tmp = g.next(p[j]);
			np.insert(np.begin(), tmp.begin(), tmp.end());
			//if (p.size() > 1 && tmp.size() > 1)
			//	proper_nest = false;
		}

		sort(pp.begin(), pp.end());
		pp.resize(unique(pp.begin(), pp.end()) - pp.begin());
		sort(np.begin(), np.end());
		np.resize(unique(np.begin(), np.end()) - np.begin());

		n = np;
		p = pp;
	}

	if (p.size() > 1)
	{
		result = "[...";
		for (int j = 0; j < (int)p.size(); j++)
		{
			if (j != 0)
				result += "[]...";

			if (g.transitions[p[j].index].behavior == hse::transition::active)
				result += export_composition(g.transitions[p[j].index].local_action, v).to_string();
			else
				result += "[" + export_expression_xfactor(g.transitions[p[j].index].local_action, v).to_string() + "]";
		}
		result += "] ; ";
	}
	else if (p.size() == 1 && g.transitions[p[0].index].behavior == hse::transition::active)
		result =  export_composition(g.transitions[p[0].index].local_action, v).to_string() + " ; ";
	else if (p.size() == 1 && g.next(g.prev(p[0])).size() > 1)
		result = "[" + export_expression_xfactor(g.transitions[p[0].index].local_action, v).to_string() + " -> ";
	else if (p.size() == 1)
		result = "[" + export_expression_xfactor(g.transitions[p[0].index].local_action, v).to_string() + "] ; ";

	if (n.size() > 1)
	{
		result += "[";
		for (int j = 0; j < (int)n.size(); j++)
		{
			if (j != 0)
				result += "[]";

			if (n[j] == i)
				result += " ";

			if (g.transitions[n[j].index].behavior == hse::transition::active)
				result += "1->" + export_composition(g.transitions[n[j].index].local_action, v).to_string() + "...";
			else
				result += export_expression_xfactor(g.transitions[n[j].index].local_action, v).to_string() + "->...";

			if (n[j] == i)
				result += " ";
		}
		result += "]";
	}
	else if (n.size() == 1 && g.transitions[n[0].index].behavior == hse::transition::active)
		result += export_composition(g.transitions[n[0].index].local_action, v).to_string();
	else if (n.size() == 1)
		result += "[" + export_expression_xfactor(g.transitions[n[0].index].local_action, v).to_string() + "]";

	return result;
}
