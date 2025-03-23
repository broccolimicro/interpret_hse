#include "import_dot.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

hse::iterator import_hse(const parse_dot::node_id &syntax, map<string, hse::iterator> &nodes, hse::graph &g, tokenizer *tokens, bool define, bool squash_errors)
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

void import_hse(const parse_dot::statement &syntax, hse::graph &g, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define)
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
				n.push_back(import_hse(*(parse_dot::node_id*)syntax.nodes[i], nodes, g, tokens, (syntax.statement_type == "node"), auto_define));
			else if (syntax.nodes[i]->is_a<parse_dot::graph>())
			{
				map<string, map<string, string> > sub_globals = globals;
				for (attr = attributes.begin(); attr != attributes.end(); attr++)
					sub_globals[syntax.statement_type][attr->first] = attr->second;
				import_hse(*(parse_dot::graph*)syntax.nodes[i], g, sub_globals, nodes, tokens, auto_define);
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
					c = boolean::import_cover(exp, g, 0, &temp, true);
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

void import_hse(const parse_dot::graph &syntax, hse::graph &g, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define)
{
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_hse(syntax.statements[i], g, globals, nodes, tokens, auto_define);
}

hse::graph import_hse(const parse_dot::graph &syntax, tokenizer *tokens, bool auto_define)
{
	hse::graph result;
	map<string, map<string, string> > globals;
	map<string, hse::iterator> nodes;
	if (syntax.valid)
		for (int i = 0; i < (int)syntax.statements.size(); i++)
			import_hse(syntax.statements[i], result, globals, nodes, tokens, auto_define);
	return result;
}

}
