#include "import_astg.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import_default.h>

namespace parse_astg {

BooleanExpressionImporter::BooleanExpressionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

BooleanExpressionImporter::~BooleanExpressionImporter() {
}

boolean::cover BooleanExpressionImporter::L_to_T(std::string lval, tokenizer *tokens) const {
	if (lval == "vdd") {
		return boolean::cover(1);
	} else if (lval == "gnd" or lval == "undef") {
		return boolean::cover();
	}
	if (region.back() != 0) {
		lval += "'" + std::to_string(region.back());
	}
	int uid = boolean::import_net(lval, symbols, tokens, autoDefine);
	if (uid < 0) {
		return boolean::cover();
	}
	return boolean::cover(uid, 1);
}

std::string BooleanExpressionImporter::T_to_L(boolean::cover expr, tokenizer *tokens) const {
	internal("", "sub expressions in variabe names not supported", __FILE__, __LINE__);
	return "gnd";
}

bool BooleanExpressionImporter::is_lvalue(const parse_expression::expression &syntax) const {
	return syntax.level >= expression_config::cfg->lvalueLevel;
}

std::string BooleanExpressionImporter::import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const {
	if (syntax.type < 0 or syntax.type >= (int)expression_config::cfg->literals.size() or not syntax.ptr) {
		return "gnd";
	}

	std::string type = expression_config::cfg->literals[syntax.type].first;

	if (type == "constant") {
		if (not syntax.ptr->is_a<constant>()) {
			internal("", "mismatched constant type", __FILE__, __LINE__);
			return "gnd";
		}
		std::string value = syntax.ptr->get<constant>().value;
		if (value == "vdd" or value == "gnd" or value == "undef") {
			return value;
		}
		error("", "unrecognized constant value, expected 'vdd' or 'gnd'", __FILE__, __LINE__);
		return "gnd";
	} else if (type == "literal") {
		if (not syntax.ptr->is_a<literal>()) {
			internal("", "mismatched literal type", __FILE__, __LINE__);
			return "gnd";
		}
		return syntax.ptr->get<literal>().name;
	} else if (type == "label") {
		if (not syntax.ptr->is_a<label>()) {
			internal("", "mismatched label type", __FILE__, __LINE__);
			return "gnd";
		}
		return syntax.ptr->get<label>().value;
	} else if (type == "ident") {
		if (not syntax.ptr->is_a<ident>()) {
			internal("", "mismatched ident type", __FILE__, __LINE__);
			return "gnd";
		}
		return syntax.ptr->get<ident>().value;
	}
	internal("", "unsupported literal type '" + type + "'", __FILE__, __LINE__);
	return "gnd";
}

void BooleanExpressionImporter::push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) {
	if (op.is("", "'", "", "")) { // Region
		int value = -1;
		if (args.size() == 2u) {
			std::string str = args[1].ptr->to_string("");
			value = atoi(str.c_str());
		} else {
			error("", "operator ''' expects 2 arguments, found '" + ::to_string(args.size()) + "'", __FILE__, __LINE__);
		}
		this->region.push_back(value);
	}
}

void BooleanExpressionImporter::pop_properties(parse_expression::operation op) {
	if (op.is("", "'", "", "")) { // Region
		region.pop_back();
	}
}

std::string BooleanExpressionImporter::import_modifier(parse_expression::operation op, vector<std::string> args, tokenizer *tokens) const {
	if (op.is("", "'", "", "")) { // Region
		// only affects properties
		return args[0];
	} else if (op.is("", ".", "", "")) { // Member
		std::string result = args[0];
		for (int i = 1; i < (int)args.size(); i++) {
			result += "." + args[i];
		}
		return result;
	} else if (op.is("", "[", ":", "]")) {
		std::string result = args[0];
		if (args.size() > 1u) {
			result += "[" + args[1];
			for (int i = 2; i < (int)args.size(); i++) {
				result += ":" + args[i];
			}
			result += "]";
		}
		return result;
	}
	internal("", "sub expressions in variabe names not supported", __FILE__, __LINE__);
	return "gnd";
}

boolean::cover BooleanExpressionImporter::import_unary(parse_expression::operation op, boolean::cover expr, tokenizer *tokens) const {
	if (op.is("~", "", "", "")) {
		return ~expr;
	} else if (op.is("?", "", "", "")) {
		//return expr.nulled();
		return boolean::cover();
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return expr;
}

boolean::cover BooleanExpressionImporter::import_binary(parse_expression::operation op, boolean::cover left, boolean::cover right, tokenizer *tokens) const {
	if (op.is("", "", "|", "")) {
		return left | right;
	} else if (op.is("", "", "&", "")) {
		return left & right;
	} else if (op.is("", "", "^", "")) {
		return left ^ right;
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}

boolean::cover BooleanExpressionImporter::import_modifier(parse_expression::operation op, vector<boolean::cover> args, tokenizer *tokens) const {
	if (op.is("", "'", "", "")) { // Region
		// only affects properties
		return args[0];
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return boolean::cover();
}

boolean::cover import_cover(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return BooleanExpressionImporter(nets, region, auto_define).import_expression(syntax, tokens);
}

boolean::cube import_cube(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	boolean::cover result = BooleanExpressionImporter(nets, region, auto_define).import_expression(syntax, tokens);
	if (result.cubes.size() > 1) {
		if (tokens != nullptr) {
			tokens->error("expected cube, found cover", __FILE__, __LINE__);
		} else {
			error("", "expected cube, found cover", __FILE__, __LINE__);
		}
		return boolean::cube();
	} else if (result.cubes.empty()) {
		return boolean::cube(0);
	}
	return result.cubes[0];
}

BooleanCompositionImporter::BooleanCompositionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

BooleanCompositionImporter::~BooleanCompositionImporter() {
}

boolean::cube BooleanCompositionImporter::import_assignment(const assignment &syntax, tokenizer *tokens) const {
	BooleanExpressionImporter in(symbols, region.back(), autoDefine);

	if (syntax.operation.empty() or syntax.left.size() != 1u) {
		error("", "malformed assignment", __FILE__, __LINE__);
		return boolean::cube();
	}

	std::string lval = in.import_lvalue(syntax.left[0], tokens);
	if (region.back() != 0) {
		lval += "'" + std::to_string(region.back());
	}
	int uid = boolean::import_net(lval, symbols, tokens, autoDefine);
	if (uid < 0) {
		return boolean::cube();
	}

 	if (syntax.operation == "+") {
		return boolean::cube(uid, 1);
	} else if (syntax.operation == "-") {
		return boolean::cube(uid, 0);
	} else if (syntax.operation == "~") {
		return boolean::cube(uid, -1);
	} else if (syntax.operation == "=") {
		std::string rval = in.import_lvalue(syntax.right, tokens);
		if (rval == "vdd") {
			return boolean::cube(uid, 1);
		} else if (rval == "gnd") {
			return boolean::cube(uid, 0);
		}
		internal("", "unsupported constant type", __FILE__, __LINE__);
		return boolean::cube();
	}
	internal("", "unsupported assignment operation", __FILE__, __LINE__);
	return boolean::cube();
}

boolean::cover BooleanCompositionImporter::import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const {
	if (syntax.type < 0 or syntax.type >= (int)expression_config::cfg->literals.size() or not syntax.ptr) {
		return boolean::cover();
	}

	return boolean::cover(import_assignment(syntax.ptr->get<assignment>(), tokens));
}

void BooleanCompositionImporter::push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) {
	if (op.is("", "'", "", "")) { // Region
		int value = -1;
		if (args.size() == 2u) {
			std::string str = args[1].ptr->to_string("");
			value = atoi(str.c_str());
		} else {
			error("", "operator ''' expects 2 arguments, found '" + ::to_string(args.size()) + "'", __FILE__, __LINE__);
		}
		this->region.push_back(value);
	}
}

void BooleanCompositionImporter::pop_properties(parse_expression::operation op) {
	if (op.is("", "'", "", "")) { // Region
		region.pop_back();
	}
}

boolean::cover BooleanCompositionImporter::import_modifier(parse_expression::operation op, vector<boolean::cover> args, tokenizer *tokens) const {
	if (op.is("", "'", "", "")) {
		return args[0];
	}
	return parse_expression::Importer<boolean::cover>::import_modifier(op, args, tokens);
}

boolean::cover BooleanCompositionImporter::import_binary(parse_expression::operation op, boolean::cover left, boolean::cover right, tokenizer *tokens) const {
	if (op.is("", "", ":", "")) {
		return boolean::choice(left, right);
	} else if (op.is("", "", ",", "")) {
		return boolean::parallel(left, right);
	}
	internal("", "unrecognized operation", __FILE__, __LINE__);
	return left;
}

boolean::cube import_boolean_assignment(const assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return BooleanCompositionImporter(nets, region, auto_define).import_assignment(syntax, tokens);
}

boolean::cover import_boolean_choice(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	return BooleanCompositionImporter(nets, region, auto_define).import_expression(syntax, tokens);
}

boolean::cube import_boolean_parallel(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region, bool auto_define) {
	boolean::cover result = BooleanCompositionImporter(nets, region, auto_define).import_expression(syntax, tokens);
	if (result.cubes.size() > 1) {
		if (tokens != nullptr) {
			tokens->error("expected cube, found cover", __FILE__, __LINE__);
		} else {
			error("", "expected cube, found cover", __FILE__, __LINE__);
		}
		return boolean::cube();
	} else if (result.cubes.empty()) {
		return boolean::cube(0);
	}
	return result.cubes[0];
}

}

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
			guard = parse_astg::import_cover(syntax.guard, dst, tokens, 0, false);
		}
		if (syntax.assign.valid) {
			action = import_boolean_choice(syntax.assign, dst, tokens, 0, false);
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
			dst.places[loc->second.index].predicate = parse_astg::import_cover(syntax.predicate[i].second, dst, tokens, 0, false);
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
			dst.places[loc->second.index].effective = parse_astg::import_cover(syntax.effective[i].second, dst, tokens, 0, false);
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
			rst.encodings = parse_astg::import_boolean_parallel(syntax.marking[i].first, dst, tokens, 0, false);

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
