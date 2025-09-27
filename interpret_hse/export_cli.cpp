#include "export_cli.h"

#include <hse/expression.h>

namespace hse {

/*// TODO this doesn't know how to handle non-deterministic choice yet.
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
					c->branches[i].first = hse::emit_expression(boolean::cube(), v);

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

string export_node(petri::iterator i, const hse::graph &g)
{
	vector<petri::iterator> n, p;
	if (i.type == hse::place::type) {
		p.push_back(i);
		n.push_back(i);
	} else {
		p = g.prev(i);
		n = g.next(i);
	}

	string result = "";

	if (p.size() > 1) {
		result += "(...";
	}
	for (int j = 0; j < (int)p.size(); j++)
	{
		if (j != 0)
			result += "||...";

		vector<petri::iterator> pp = g.prev(p[j]);
		if (pp.size() > 1) {
			result += "[...";
		}
		for (int j = 0; j < (int)pp.size(); j++) {
			if (j != 0)
				result += "[]...";

			if (!g.transitions[pp[j].index].guard.is_tautology())
				result += "[" + hse::emit_expression_xfactor(g.transitions[pp[j].index].guard, g) + "]; ";
			
			result += hse::emit_composition(g.transitions[pp[j].index].local_action, g);
		}
		if (pp.size() > 1) {
			result += "]";
		}
	}
	if (p.size() > 1) {
		result += ")";
	}



	if (i.type == hse::place::type) {
		result += "; <here> ";
	} else {
		if (not g.transitions[i.index].guard.is_tautology()) {
			result += ";[" + hse::emit_expression_xfactor(g.transitions[i.index].guard, g) + "]; <here> ";
		} else {
			result += "; <here> ";
		}

		result +=  hse::emit_composition(g.transitions[i.index].local_action, g) + ";";
	}



	if (n.size() > 1) {
		result += "(";
	}
	for (int j = 0; j < (int)n.size(); j++)
	{
		if (j != 0)
			result += "...||";

		vector<petri::iterator> nn = g.next(n[j]);
		if (nn.size() > 1) {
			result += "[";
		}
		for (int j = 0; j < (int)nn.size(); j++) {
			if (j != 0)
				result += "...[]";

			if (!g.transitions[nn[j].index].guard.is_tautology())
				result += hse::emit_expression_xfactor(g.transitions[nn[j].index].guard, g) + "->";
			else
				result += "1->";
			
			result += hse::emit_composition(g.transitions[nn[j].index].local_action, g);
		}
		if (nn.size() > 1) {
			result += "...]";
		}
	}
	if (n.size() > 1) {
		result += "...)";
	}

	return result;
}

void print_conflicts(const hse::encoder &enc) {
	for (int sense = -1; sense < 2; sense++) {
		for (auto i = enc.conflicts.begin(); i != enc.conflicts.end(); i++) {
			if (i->sense == sense) {
				printf("T%d.%d\t...%s...   conflicts with:\n", i->index.index, i->index.term, export_node(i->index.iter(), *enc.base).c_str());

				for (auto j = i->region.begin(); j != i->region.end(); j++) {
					printf("\t%s\t...%s...\n", j->to_string().c_str(), export_node(*j, *enc.base).c_str());
				}
				printf("\n");
			}
		}
		printf("\n");
	}
}

}
