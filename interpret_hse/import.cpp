/*
 * import.cpp
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include "import.h"
#include <interpret_boolean/import.h>

namespace hse {

hse::iterator import_hse(const parse_astg::node &syntax, ucs::variable_set &variables, hse::graph &g, map<string, hse::iterator> &ids, tokenizer *tokens)
{
	hse::iterator i(-1,-1);
	if (syntax.id.size() > 0) {
		i.type = hse::transition::type;
		i.index = std::stoi(syntax.id);
	} else if (syntax.place.size() > 0 && syntax.place[0] == 'p') {
		i.type = hse::place::type;
		i.index = std::stoi(syntax.place.substr(1));
	} else if (tokens != NULL) {
		tokens->load(&syntax);
		tokens->error("Undefined node", __FILE__, __LINE__);
		return i;
	} else {
		error("", "Undefined node \"" + syntax.to_string() + "\"", __FILE__, __LINE__);
		return i;
	}
	
	auto created = ids.insert(pair<string, hse::iterator>(syntax.to_string(), i));
	if (created.second && i.type == hse::transition::type)
	{
		boolean::cover guard = 1;
		boolean::cover action = 1;
		if (syntax.guard.valid) {
			guard = import_cover(syntax.guard, variables, 0, tokens, false);
		}
		if (syntax.assign.valid) {
			action = import_cover(syntax.assign, variables, 0, tokens, false);
		}
		
		g.create_at(hse::transition(guard, action), i.index);
	} else if (created.second) {
		g.create_at(hse::place(), i.index);
	}

	return i;
}

void import_hse(const parse_astg::arc &syntax, ucs::variable_set &variables, hse::graph &g, map<string, hse::iterator> &ids, tokenizer *tokens)
{
	hse::iterator base = import_hse(syntax.nodes[0], variables, g, ids, tokens);
	for (int i = 1; i < (int)syntax.nodes.size(); i++)
	{
		hse::iterator next = import_hse(syntax.nodes[i], variables, g, ids, tokens);
		g.connect(base, next);
	}
}

hse::graph import_hse(const parse_astg::graph &syntax, ucs::variable_set &variables, tokenizer *tokens)
{
	hse::graph result;
	map<string, hse::iterator> ids;
	for (int i = 0; i < (int)syntax.inputs.size(); i++)
		define_variables(syntax.inputs[i], variables, 0, tokens, true, false);

	for (int i = 0; i < (int)syntax.outputs.size(); i++)
		define_variables(syntax.outputs[i], variables, 0, tokens, true, false);

	for (int i = 0; i < (int)syntax.internal.size(); i++)
		define_variables(syntax.internal[i], variables, 0, tokens, true, false);

	for (int i = 0; i < (int)syntax.arcs.size(); i++)
		import_hse(syntax.arcs[i], variables, result, ids, tokens);

	for (int i = 0; i < (int)syntax.predicate.size(); i++)
	{
		map<string, hse::iterator>::iterator loc = ids.find(syntax.predicate[i].first.to_string());
		if (loc != ids.end())
			result.places[loc->second.index].predicate = import_cover(syntax.predicate[i].second, variables, 0, tokens, false);
		else if (tokens != NULL)
		{
			tokens->load(&syntax.predicate[i].first);
			tokens->error("Undefined node", __FILE__, __LINE__);
		}
		else
			error("", "Undefined node \"" + syntax.predicate[i].first.to_string() + "\"", __FILE__, __LINE__);
	}

	for (int i = 0; i < (int)syntax.effective.size(); i++)
	{
		map<string, hse::iterator>::iterator loc = ids.find(syntax.effective[i].first.to_string());
		if (loc != ids.end())
			result.places[loc->second.index].effective = import_cover(syntax.effective[i].second, variables, 0, tokens, false);
		else if (tokens != NULL)
		{
			tokens->load(&syntax.effective[i].first);
			tokens->error("Undefined node", __FILE__, __LINE__);
		}
		else
			error("", "Undefined node \"" + syntax.effective[i].first.to_string() + "\"", __FILE__, __LINE__);
	}

	for (int i = 0; i < (int)syntax.marking.size(); i++)
	{
		hse::state rst;
		if (syntax.marking[i].first.valid)
			rst.encodings = import_cube(syntax.marking[i].first, variables, 0, tokens, false);

		for (int j = 0; j < (int)syntax.marking[i].second.size(); j++)
		{
			hse::iterator loc = import_hse(syntax.marking[i].second[j], variables, result, ids, tokens);
			if (loc.type == hse::place::type && loc.index >= 0)
				rst.tokens.push_back(loc.index);
		}
		result.reset.push_back(rst);
	}

	for (int i = 0; i < (int)syntax.arbiter.size(); i++) {
		hse::iterator loc = import_hse(syntax.arbiter[i], variables, result, ids, tokens);
		if (loc.type == hse::place::type and loc.index >= 0) {
			result.places[loc.index].arbiter = true;
		}
	}

	return result;
}

hse::iterator import_hse(const parse_dot::node_id &syntax, map<string, hse::iterator> &nodes, ucs::variable_set &variables, hse::graph &g, tokenizer *tokens, bool define, bool squash_errors)
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

map<string, string> import_hse(const parse_dot::attribute_list &syntax, tokenizer *tokens)
{
	map<string, string> result;
	if (syntax.valid)
		for (vector<parse_dot::assignment_list>::const_iterator l = syntax.attributes.begin(); l != syntax.attributes.end(); l++)
			if (l->valid)
				for (vector<parse_dot::assignment>::const_iterator a = l->as.begin(); a != l->as.end(); a++)
					result.insert(pair<string, string>(a->first, a->second));

	return result;
}

void import_hse(const parse_dot::statement &syntax, hse::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	map<string, string> attributes = import_hse(syntax.attributes, tokens);
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
				n.push_back(import_hse(*(parse_dot::node_id*)syntax.nodes[i], nodes, variables, g, tokens, (syntax.statement_type == "node"), auto_define));
			else if (syntax.nodes[i]->is_a<parse_dot::graph>())
			{
				map<string, map<string, string> > sub_globals = globals;
				for (attr = attributes.begin(); attr != attributes.end(); attr++)
					sub_globals[syntax.statement_type][attr->first] = attr->second;
				import_hse(*(parse_dot::graph*)syntax.nodes[i], g, variables, sub_globals, nodes, tokens, auto_define);
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

				bool is_guard = false;
				if (temp.decrement(__FILE__, __LINE__))
				{
					is_guard = true;
					temp.next();

					temp.increment(l, true);
					temp.expect(l, "]");
				}

				if (temp.decrement(__FILE__, __LINE__))
				{
					parse_expression::expression exp(temp);
					c = import_cover(exp, variables, 0, &temp, true);
				}

				if (is_guard && temp.decrement(__FILE__, __LINE__))
					temp.next();

				for (int i = 0; i < (int)n.size(); i++)
				{
					if (n[i].type == hse::transition::type)
					{
						if (is_guard) {
							g.transitions[n[i].index].guard = c;
						} else {
							g.transitions[n[i].index].local_action = c;
						}
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

void import_hse(const parse_dot::graph &syntax, hse::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_hse(syntax.statements[i], g, variables, globals, nodes, tokens, auto_define);
}

hse::graph import_hse(const parse_dot::graph &syntax, ucs::variable_set &variables, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	map<string, map<string, string> > globals;
	map<string, hse::iterator> nodes;
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_hse(syntax.statements[i], result, variables, globals, nodes, tokens, auto_define);
	return result;
}

hse::graph import_hse(const parse_expression::expression &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(import_cover(syntax, variables, default_id, tokens, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(hse::state(vector<petri::token>(1, petri::token(b.index)), boolean::cube(1)));
	result.sink.push_back(hse::state(vector<petri::token>(1, petri::token(e.index)), boolean::cube(1)));
	return result;
}

hse::graph import_hse(const parse_expression::assignment &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	hse::iterator b = result.create(hse::place());
	hse::iterator t = result.create(hse::transition(1, import_cover(syntax, variables, default_id, tokens, auto_define)));
	hse::iterator e = result.create(hse::place());

	result.connect(b, t);
	result.connect(t, e);

	result.source.push_back(hse::state(vector<petri::token>(1, petri::token(b.index)), boolean::cube(1)));
	result.sink.push_back(hse::state(vector<petri::token>(1, petri::token(e.index)), boolean::cube(1)));
	return result;
}

hse::graph import_hse(const parse_chp::composition &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
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
			result.merge(composition, import_hse(syntax.branches[i].sub, variables, default_id, tokens, auto_define));
		else if (syntax.branches[i].ctrl.valid)
			result.merge(composition, import_hse(syntax.branches[i].ctrl, variables, default_id, tokens, auto_define));
		else if (syntax.branches[i].assign.valid)
			result.merge(composition, import_hse(syntax.branches[i].assign, variables, default_id, tokens, auto_define));

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

hse::graph import_hse(const parse_chp::control &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define)
{
	if (syntax.region != "")
		default_id = atoi(syntax.region.c_str());

	hse::graph result;

	for (int i = 0; i < (int)syntax.branches.size(); i++)
	{
		hse::graph branch;
		if (syntax.branches[i].first.valid and import_cover(syntax.branches[i].first, variables, default_id, tokens, auto_define) != 1)
			branch.merge(hse::sequence, import_hse(syntax.branches[i].first, variables, default_id, tokens, auto_define));
		if (syntax.branches[i].second.valid)
			branch.merge(hse::sequence, import_hse(syntax.branches[i].second, variables, default_id, tokens, auto_define));

		result.merge(hse::choice, branch);
	}

	if ((!syntax.deterministic || syntax.repeat) && syntax.branches.size() > 0)
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
				result.erase(loc);
				if (sm.index > loc.index)
					sm.index--;
			}
		}

		result.source.clear();
		result.source.push_back(hse::state(vector<petri::token>(1, petri::token(sm.index)), boolean::cube(1)));
	}

	if (!syntax.deterministic)
		for (int i = 0; i < (int)result.source.size(); i++)
			for (int j = 0; j < (int)result.source[i].tokens.size(); j++)
				result.places[result.source[i].tokens[j].index].arbiter = true;

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

		if (!repeat.is_null())
		{
			hse::iterator guard = result.create(hse::transition(repeat));
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
