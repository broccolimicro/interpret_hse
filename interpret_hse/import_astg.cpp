#include "import_astg.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import.h>

namespace hse {

hse::iterator import_hse(hse::graph &dst, const parse_astg::node &syntax, map<string, hse::iterator> &ids, tokenizer *tokens)
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
			guard = boolean::import_cover(syntax.guard, dst, 0, tokens, false);
		}
		if (syntax.assign.valid) {
			action = boolean::import_cover(syntax.assign, dst, 0, tokens, false);
		}
		
		dst.create_at(hse::transition(1, guard, action), i.index);
	} else if (created.second) {
		dst.create_at(hse::place(), i.index);
	}

	return i;
}

void import_hse(hse::graph &dst, const parse_astg::arc &syntax, map<string, hse::iterator> &ids, tokenizer *tokens)
{
	hse::iterator base = import_hse(dst, syntax.nodes[0], ids, tokens);
	for (int i = 1; i < (int)syntax.nodes.size(); i++)
	{
		hse::iterator next = import_hse(dst, syntax.nodes[i], ids, tokens);
		dst.connect(base, next);
	}
}

void import_hse(hse::graph &dst, const parse_astg::graph &syntax, tokenizer *tokens)
{
	map<string, hse::iterator> ids;
	for (int i = 0; i < (int)syntax.inputs.size(); i++)
		boolean::import_net(syntax.inputs[i].to_string(), dst, tokens, true);

	for (int i = 0; i < (int)syntax.outputs.size(); i++)
		boolean::import_net(syntax.outputs[i].to_string(), dst, tokens, true);

	for (int i = 0; i < (int)syntax.internal.size(); i++)
		boolean::import_net(syntax.internal[i].to_string(), dst, tokens, true);

	for (int i = 0; i < (int)syntax.arcs.size(); i++)
		import_hse(dst, syntax.arcs[i], ids, tokens);

	for (int i = 0; i < (int)syntax.predicate.size(); i++)
	{
		map<string, hse::iterator>::iterator loc = ids.find(syntax.predicate[i].first.to_string());
		if (loc != ids.end())
			dst.places[loc->second.index].predicate = boolean::import_cover(syntax.predicate[i].second, dst, 0, tokens, false);
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
			dst.places[loc->second.index].effective = boolean::import_cover(syntax.effective[i].second, dst, 0, tokens, false);
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
			rst.encodings = boolean::import_cube(syntax.marking[i].first, dst, 0, tokens, false);

		for (int j = 0; j < (int)syntax.marking[i].second.size(); j++)
		{
			hse::iterator loc = import_hse(dst, syntax.marking[i].second[j], ids, tokens);
			if (loc.type == hse::place::type && loc.index >= 0)
				rst.tokens.push_back(loc.index);
		}
		dst.reset.push_back(rst);
	}

	for (int i = 0; i < (int)syntax.arbiter.size(); i++) {
		hse::iterator loc = import_hse(dst, syntax.arbiter[i], ids, tokens);
		if (loc.type == hse::place::type and loc.index >= 0) {
			dst.places[loc.index].arbiter = true;
		}
	}
}

}
