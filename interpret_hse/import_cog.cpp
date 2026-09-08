#include "import_cog.h"
#include "import_expr.h"

#include <common/standard.h>
#include <interpret_boolean/import_default.h>

namespace parse_cog {

BooleanExpressionImporter::BooleanExpressionImporter(ucs::Netlist symbols, int region, bool autoDefine) : symbols(symbols) {
	this->region.push_back(region);
	this->autoDefine = autoDefine;
}

BooleanExpressionImporter::~BooleanExpressionImporter() {
}

boolean::cover BooleanExpressionImporter::L_to_T(std::string lval, tokenizer *tokens) const {
	if (lval == "vdd") {
		return boolean::cover(1);
	} else if (lval == "gnd") {
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
		std::string value = syntax.ptr->get<constant>().value;
		if (value == "vdd" or value == "gnd") {
			return value;
		}
		error("", "unrecognized constant value, expected 'vdd' or 'gnd'", __FILE__, __LINE__);
		return "gnd";
	} else if (type == "literal") {
		return syntax.ptr->get<literal>().name;
	} else if (type == "label") {
		return syntax.ptr->get<label>().value;
	} else if (type == "ident") {
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


hse::segment import_segment(hse::graph &dst, const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define) {
	hse::segment result(false);
	hse::transition n;
	if (assume) {
		n.assume = import_cover(syntax, dst, tokens, default_id, auto_define);
	} else {
		n.guard = import_cover(syntax, dst, tokens, default_id, auto_define);
		result.cond = n.guard;
	}

	hse::iterator t = dst.create(n);
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

hse::segment import_segment(hse::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	hse::segment result(true);
	petri::iterator t = dst.create(hse::transition(1, 1, import_boolean_assignment(syntax, dst, tokens, default_id, auto_define)));
	result.nodes = petri::segment({{t}}, {{t}});
	return result;
}

}

namespace hse {

hse::segment import_segment(hse::graph &dst, const parse_cog::composition &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	bool arbiter = false;
	bool synchronizer = false;

	petri::Composition composition = petri::PARALLEL;
	if (syntax.level == parse_cog::composition::SEQUENCE or syntax.level == parse_cog::composition::INTERNAL_SEQUENCE) {
		composition = petri::SEQUENCE;
	} else if (syntax.level == parse_cog::composition::CONDITION) {
		composition = petri::CHOICE;
	} else if (syntax.level == parse_cog::composition::CHOICE) {
		composition = petri::CHOICE;
		arbiter = true;
	} else if (syntax.level == parse_cog::composition::PARALLEL) {
		composition = petri::PARALLEL;
	}

	hse::segment result(composition != petri::CHOICE);
	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		auto branch = import_segment(dst, syntax.branches[i].get(), default_id, tokens, auto_define);
		result = compose(dst, composition, result, branch);
	}

	if (syntax.branches.size() == 0) {
		petri::iterator from = dst.create(hse::place());

		result.nodes.source.push_back({from});
		result.nodes.sink.push_back({from});
	}

	// At this point, source and sink are likely *transitions*.
	// If this is a conditional composition, then they need to be
	// the places *before* those transitions. And if there aren't
	// any places before those transitions then we need to create
	// them

	if (result.nodes.source.size() > 1u and composition == petri::CHOICE) {
		petri::iterator from = dst.create(hse::place());
		dst.connect({{from}}, result.nodes.source);
		result.nodes.source = petri::bound({{from}});
	}

	if (result.nodes.sink.size() > 1u and composition == petri::CHOICE) {
		petri::iterator to = dst.create(hse::place());
		dst.connect(result.nodes.sink, {{to}});
		result.nodes.sink = petri::bound({{to}});
	}

	/*if ((int)syntax.branches.size() > 1) {
		cout << "BEFORE" << endl;
		cout << syntax.to_string() << endl << endl;
		cout << "Source: ";
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;
		cout << "Sink: ";
		for (auto i = result.sink.begin(); i != result.sink.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << export_astg(result).to_string() << endl << endl;
	}*/

	if ((arbiter or synchronizer) and syntax.branches.size() > 1u) {
		for (auto i = result.nodes.source.begin(); i != result.nodes.source.end(); i++) {
			for (auto j = i->begin(); j != i->end(); j++) {
				if (j->type != hse::place::type) {
					printf("%s:%d: internal: expected place\n", __FILE__, __LINE__);
				} else {
					if (arbiter) {
						dst.places[j->index].arbiter = true;
					}
					if (synchronizer) {
						dst.places[j->index].synchronizer = true;
					}
				}
			}
		}
	}

	if (result.loop and composition == petri::CHOICE and not arbiter and not result.nodes.source.empty()) {
		boolean::cover skipCond = ~result.cond;
		if (not skipCond.is_null()) {
			petri::iterator arrow = dst.create(hse::place());
			for (auto i = result.nodes.source.begin(); i != result.nodes.source.end(); i++) {
				petri::iterator skip = dst.create(hse::transition(1, skipCond));
				dst.connect(*i, skip);
				dst.connect(skip, arrow);
			}

			dst.connect(result.nodes.sink, {{arrow}});
			result.nodes.sink = petri::bound({{arrow}});
		}
		result.loop = false;
	}

	/*if ((int)syntax.branches.size() > 1) {
		cout << "AFTER" << endl;
		cout << "Source: ";
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;
		cout << "Sink: ";
		for (auto i = result.sink.begin(); i != result.sink.end(); i++) {
			for (auto j = i->tokens.begin(); j != i->tokens.end(); j++) {
				cout << "p" << j->index << " ";
			}
			cout << endl;
		}
		cout << endl;

		cout << export_astg(result).to_string() << endl << endl;
	}*/

	return result;
}

hse::segment import_segment(hse::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	hse::segment result(true);
	if (syntax.guard.valid and import_cover(syntax.guard, dst, tokens, default_id, auto_define) != 1) {
		auto sub = import_segment(dst, syntax.guard, syntax.kind == "assume", default_id, tokens, auto_define);
		if (syntax.kind == "if") {
			if (tokens != NULL) {
				tokens->load(&syntax);
				tokens->error("if statements not supported in wire-level specifications", __FILE__, __LINE__);
			} else {
				error("", "if statements not supported in wire-level specifications", __FILE__, __LINE__);
			}
		}
		result = compose(dst, petri::SEQUENCE, result, sub);
	}
	if (syntax.action.valid) {
		auto sub = import_segment(dst, syntax.action, default_id, tokens, auto_define);
		result = compose(dst, petri::SEQUENCE, result, sub);
	}

	if (syntax.kind == "while" and not result.nodes.source.empty()) {
		result.loop = true;
		petri::iterator link = dst.create(hse::place());
		dst.connect({{link}}, result.nodes.source);
		dst.connect(result.nodes.sink, {{link}});
		result.nodes.sink.clear();
		if (not result.nodes.reset.empty()) {
			result.nodes.source = result.nodes.reset;
			result.nodes.reset.clear();
		} else {
			result.nodes.source = petri::bound({{link}});
		}
	}

	return result;
}

hse::segment import_segment(hse::graph &dst, const parse_cog::declaration &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	// TODO(edward.bingham) handle the variable creation
	//dst.create(chp::variable());

	return import_segment(dst, syntax.expr, default_id, tokens, auto_define);
}

hse::segment import_segment(hse::graph &dst, const parse::syntax *syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax != nullptr and syntax->valid) {
		if (syntax->is_a<parse_cog::composition>()) {
			return import_segment(dst, syntax->get<parse_cog::composition>(), default_id, tokens, auto_define);
		} else if (syntax->is_a<parse_cog::control>()) {
			return import_segment(dst, syntax->get<parse_cog::control>(), default_id, tokens, auto_define);
		} else if (syntax->is_a<parse_cog::assignment>()) {
			return import_segment(dst, syntax->get<parse_cog::assignment>(), default_id, tokens, auto_define);
		} else if (syntax->is_a<parse_cog::declaration>()) {
			return import_segment(dst, syntax->get<parse_cog::declaration>(), default_id, tokens, auto_define);
		}
	}
	return hse::segment(true);
}

void import_hse(hse::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define) {
	petri::segment result = import_segment(dst, syntax, 0, tokens, auto_define).nodes;
	if (not result.reset.empty()) {
		result.source = result.reset;
		result.reset.clear();
	}

	vector<hse::state> reset;
	for (auto i = result.source.begin(); i != result.source.end(); i++) {
		hse::state rst;
		for (auto j = i->begin(); j != i->end(); j++) {
			if (j->type == hse::transition::type) {
				vector<petri::iterator> p = dst.prev(*j);
				if (p.empty()) {
					p.push_back(dst.create(hse::place()));
					dst.connect(p.back(), *j);
				}
				for (auto k = p.begin(); k != p.end(); k++) {
					rst.tokens.push_back(petri::token(k->index));
				}
			} else {
				rst.tokens.push_back(petri::token(j->index));
			}
		}
		reset.push_back(rst);
	}

	if (dst.reset.empty()) {
		dst.reset = reset;
	} else {
		vector<hse::state> prev = dst.reset;
		dst.reset.clear();

		for (auto i = prev.begin(); i != prev.end(); i++) {
			for (auto j = reset.begin(); j != reset.end(); j++) {
				dst.reset.push_back(hse::state::merge(*i, *j));
			}
		}
	}
}

}
