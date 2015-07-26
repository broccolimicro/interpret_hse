/*
 * import.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "import.h"
#include <interpret_boolean/import.h>

hse::iterator import_graph(const parse_dot::node_id &syntax, map<string, hse::iterator> &nodes, ucs::variable_set &variables, hse::graph &g, tokenizer *tokens, bool define, bool squash_errors)
{
	if (syntax.valid && syntax.id.size() > 0)
	{
		map<string, hse::iterator>::iterator i = nodes.find(syntax.id);
		if (i != nodes.end())
		{
			if (define && !squash_errors)
			{
				if (tokens != NULL)
				{
					tokens->load(&syntax);
					tokens->error("redefinition of node '" + syntax.id + "'", __FILE__, __LINE__);
					if (tokens->load(syntax.id))
						tokens->note("previously defined here", __FILE__, __LINE__);
				}
				else
					error(syntax.to_string(), "redefinition of node '" + syntax.id + "'", __FILE__, __LINE__);
			}
			return i->second;
		}
		else
		{

			if (!define && !squash_errors)
			{
				if (tokens != NULL)
				{
					tokens->load(&syntax);
					tokens->error("node '" + syntax.id + "' not yet defined", __FILE__, __LINE__);
				}
				else
					error(syntax.to_string(), "node '" + syntax.id + "' not yet defined", __FILE__, __LINE__);
			}
			else if (tokens != NULL)
				tokens->save(syntax.id, &syntax);

			if (syntax.id[0] == 'P')
			{
				hse::iterator result = g.create(hse::place());
				nodes.insert(pair<string, hse::iterator>(syntax.id, result));
				return result;
			}
			else if (syntax.id[0] == 'T')
			{
				hse::iterator result = g.create(hse::transition());
				nodes.insert(pair<string, hse::iterator>(syntax.id, result));
				return result;
			}
			else
			{
				if (tokens != NULL)
				{
					tokens->load(&syntax);
					tokens->error("Unrecognized node type '" + syntax.id + "'", __FILE__, __LINE__);
				}
				else
					error(syntax.to_string(), "Unrecognized node type '" + syntax.id + "'", __FILE__, __LINE__);
				return hse::iterator();
			}
		}
	}
	else
		return hse::iterator();
}

map<string, string> import_graph(const parse_dot::attribute_list &syntax, tokenizer *tokens)
{
	map<string, string> result;
	if (syntax.valid)
		for (vector<parse_dot::assignment_list>::const_iterator l = syntax.attributes.begin(); l != syntax.attributes.end(); l++)
			if (l->valid)
				for (vector<parse_dot::assignment>::const_iterator a = l->as.begin(); a != l->as.end(); a++)
					result.insert(pair<string, string>(a->first, a->second));

	return result;
}

void import_graph(const parse_dot::statement &syntax, hse::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	map<string, string> attributes = import_graph(syntax.attributes, tokens);
	map<string, string>::iterator attr;

	if (syntax.statement_type == "attribute")
	{
		for (attr = attributes.begin(); attr != attributes.end(); attr++)
			globals[syntax.attribute_type][attr->first] = attr->second;
	}
	else
	{
		vector<hse::iterator> p;
		for (int i = 0; i < (int)syntax.nodes.size(); i++)
		{
			vector<hse::iterator> n;
			if (syntax.nodes[i]->is_a<parse_dot::node_id>())
				n.push_back(import_graph(*(parse_dot::node_id*)syntax.nodes[i], nodes, variables, g, tokens, (syntax.statement_type == "node"), auto_define));
			else if (syntax.nodes[i]->is_a<parse_dot::graph>())
			{
				map<string, map<string, string> > sub_globals = globals;
				for (attr = attributes.begin(); attr != attributes.end(); attr++)
					sub_globals[syntax.statement_type][attr->first] = attr->second;
				import_graph(*(parse_dot::graph*)syntax.nodes[i], g, variables, sub_globals, nodes, tokens, auto_define);
			}

			attr = attributes.find("label");
			if (attr == attributes.end())
				attr = globals[syntax.statement_type].find("label");

			boolean::cover c = 1;
			if (attr != attributes.end() && attr != globals[syntax.statement_type].end() && attr->second.size() != 0)
			{
				tokenizer temp;
				parse_expression::expression::register_syntax(temp);
				temp.insert(attr->first, attr->second);

				tokenizer::level l = temp.increment(true);
				temp.expect<parse_expression::expression>();

				temp.increment(false);
				temp.expect("[");

				int behvior = hse::transition::active;
				if (temp.decrement(__FILE__, __LINE__))
				{
					behvior = hse::transition::passive;
					temp.next();

					temp.increment(l, true);
					temp.expect(l, "]");
				}

				if (temp.decrement(__FILE__, __LINE__))
				{
					parse_expression::expression exp(temp);
					c = import_cover(exp, variables, 0, &temp, true);
				}

				if (behvior == hse::transition::passive && temp.decrement(__FILE__, __LINE__))
					temp.next();

				for (int i = 0; i < (int)n.size(); i++)
				{
					if (n[i].type == hse::transition::type)
					{
						g.transitions[n[i].index].behavior = behvior;
						g.transitions[n[i].index].local_action = c;
					}
					else if (n[i].type == hse::place::type)
					{
						g.places[n[i].index].predicate = c;
					}
				}
			}

			if (i != 0)
				g.connect(p, n);

			p = n;
		}
	}
}

void import_graph(const parse_dot::graph &syntax, hse::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_graph(syntax.statements[i], g, variables, globals, nodes, tokens, auto_define);
}

hse::graph import_graph(const parse_dot::graph &syntax, ucs::variable_set &variables, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	map<string, map<string, string> > globals;
	map<string, hse::iterator> nodes;
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_graph(syntax.statements[i], result, variables, globals, nodes, tokens, auto_define);
	return result;
}

hse::graph import_graph(const parse_expression::expression &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(hse::transition::passive, import_cover(syntax, variables, default_id, tokens, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(hse::state(vector<hse::reset_token>(1, hse::reset_token(b.index, false)), vector<hse::term_index>(), boolean::cover(1)));
	result.sink.push_back(hse::state(vector<hse::reset_token>(1, hse::reset_token(e.index, false)), vector<hse::term_index>(), boolean::cover(1)));
	return result;
}

hse::graph import_graph(const parse_expression::assignment &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(hse::transition::active, import_cover(syntax, variables, default_id, tokens, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(hse::state(vector<hse::reset_token>(1, hse::reset_token(b.index, false)), vector<hse::term_index>(), boolean::cover(1)));
	result.sink.push_back(hse::state(vector<hse::reset_token>(1, hse::reset_token(e.index, false)), vector<hse::term_index>(), boolean::cover(1)));
	return result;
}

hse::graph import_graph(const parse_chp::composition &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
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
			result.merge(composition, import_graph(syntax.branches[i].sub, variables, default_id, tokens, auto_define), false);
		else if (syntax.branches[i].ctrl.valid)
			result.merge(composition, import_graph(syntax.branches[i].ctrl, variables, default_id, tokens, auto_define), false);
		else if (syntax.branches[i].assign.valid)
			result.merge(composition, import_graph(syntax.branches[i].assign, variables, default_id, tokens, auto_define), false);

		if (syntax.reset == 0 && i == 0)
			result.reset = result.source;
		else if (syntax.reset == i+1)
			result.reset = result.sink;
	}

	return result;
}

hse::graph import_graph(const parse_chp::control &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	if (syntax.region != "")
		default_id = atoi(syntax.region.c_str());

	hse::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid && import_cover(syntax.branches[i].first, variables, default_id, tokens, auto_define) != 1)
			branch.merge(hse::sequence, import_graph(syntax.branches[i].first, variables, default_id, tokens, auto_define), false);
		if (syntax.branches[i].second.valid)
			branch.merge(hse::sequence, import_graph(syntax.branches[i].second, variables, default_id, tokens, auto_define), false);
		result.merge(hse::choice, branch, false);
	}

	if (syntax.repeat && syntax.branches.size() > 0)
	{
		boolean::cover repeat = 1;
		for (int i = 0; i < (int)syntax.branches.size() && !repeat.is_null(); i++)
		{
			if (syntax.branches[i].first.valid)
			{
				if (i == 0)
					repeat = ~import_cover(syntax.branches[i].first, variables, default_id, tokens, auto_define);
				else
					repeat &= ~import_cover(syntax.branches[i].first, variables, default_id, tokens, auto_define);
			}
			else
			{
				repeat = 0;
				break;
			}
		}

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
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.source.clear();

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
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.sink.clear();

		if (!repeat.is_null())
		{
			hse::iterator guard = result.create(hse::transition(hse::transition::passive, repeat));
			result.connect(sm, guard);
			hse::iterator arrow = result.create(hse::place());
			result.connect(guard, arrow);

			result.sink.push_back(hse::state(vector<hse::reset_token>(1, hse::reset_token(arrow.index, false)), vector<hse::term_index>(), boolean::cover(1)));
		}

		if (result.reset.size() > 0)
		{
			result.source = result.reset;
			result.reset.clear();
		}
		else
			result.source.push_back(hse::state(vector<hse::reset_token>(1, hse::reset_token(sm.index, false)), vector<hse::term_index>(), boolean::cover(1)));
	}

	return result;
}
