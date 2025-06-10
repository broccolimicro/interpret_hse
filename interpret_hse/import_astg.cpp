#include "import_astg.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

hse::iterator import_hse(const parse_astg::node &syntax, hse::graph &g, map<string, hse::iterator> &ids, tokenizer *tokens)
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
			guard = boolean::import_cover(syntax.guard, g, 0, tokens, false);
		}
		if (syntax.assign.valid) {
			action = boolean::import_cover(syntax.assign, g, 0, tokens, false);
		}
		
		g.create_at(hse::transition(1, guard, action), i.index);
	} else if (created.second) {
		g.create_at(hse::place(), i.index);
	}

	return i;
}

void import_hse(const parse_astg::arc &syntax, hse::graph &g, map<string, hse::iterator> &ids, tokenizer *tokens)
{
	hse::iterator base = import_hse(syntax.nodes[0], g, ids, tokens);
	for (int i = 1; i < (int)syntax.nodes.size(); i++)
	{
		hse::iterator next = import_hse(syntax.nodes[i], g, ids, tokens);
		g.connect(base, next);
	}
}

hse::graph import_hse(const parse_astg::graph &syntax, tokenizer *tokens)
{
	hse::graph result;
	map<string, hse::iterator> ids;
	for (int i = 0; i < (int)syntax.inputs.size(); i++)
		boolean::import_net(syntax.inputs[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.outputs.size(); i++)
		boolean::import_net(syntax.outputs[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.internal.size(); i++)
		boolean::import_net(syntax.internal[i].to_string(), result, tokens, true);

	for (int i = 0; i < (int)syntax.arcs.size(); i++)
		import_hse(syntax.arcs[i], result, ids, tokens);

	for (int i = 0; i < (int)syntax.predicate.size(); i++)
	{
		map<string, hse::iterator>::iterator loc = ids.find(syntax.predicate[i].first.to_string());
		if (loc != ids.end())
			result.places[loc->second.index].predicate = boolean::import_cover(syntax.predicate[i].second, result, 0, tokens, false);
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
			result.places[loc->second.index].effective = boolean::import_cover(syntax.effective[i].second, result, 0, tokens, false);
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
			rst.encodings = boolean::import_cube(syntax.marking[i].first, result, 0, tokens, false);

		for (int j = 0; j < (int)syntax.marking[i].second.size(); j++)
		{
			hse::iterator loc = import_hse(syntax.marking[i].second[j], result, ids, tokens);
			if (loc.type == hse::place::type && loc.index >= 0)
				rst.tokens.push_back(loc.index);
		}
		result.reset.push_back(rst);
	}

	for (int i = 0; i < (int)syntax.arbiter.size(); i++) {
		hse::iterator loc = import_hse(syntax.arbiter[i], result, ids, tokens);
		if (loc.type == hse::place::type and loc.index >= 0) {
			result.places[loc.index].arbiter = true;
		}
	}

	return result;
}

}
