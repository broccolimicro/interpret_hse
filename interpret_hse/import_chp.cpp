#include "import_chp.h"
#include "import_expr.h"
#include <common/standard.h>
#include <interpret_boolean/import_default.h>

namespace parse_chp {

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
	}  else if (type == "label") {
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

petri::segment import_segment(hse::graph &dst, const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;

	int composition = petri::parallel;
	if (parse_chp::composition::precedence[syntax.level] == "||" or parse_chp::composition::precedence[syntax.level] == ",") {
		composition = petri::parallel;
	} else if (parse_chp::composition::precedence[syntax.level] == ";") {
		composition = petri::sequence;
	}

	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		petri::segment branch;
		if (syntax.branches[i].sub.valid) {
			branch = import_segment(dst, syntax.branches[i].sub, default_id, tokens, auto_define);
		} else if (syntax.branches[i].ctrl.valid) {
			branch = import_segment(dst, syntax.branches[i].ctrl, default_id, tokens, auto_define);
		} else if (syntax.branches[i].assign.valid) {
			branch = import_segment(dst, syntax.branches[i].assign, default_id, tokens, auto_define).nodes;
		} else {
			continue;
		}

		result = dst.compose(composition, result, branch);

		if (syntax.reset == 0 and i == 0) {
			result.reset = result.source;
		} else if (syntax.reset == i+1) {
			result.reset = result.sink;
		}
	}

	if (syntax.branches.size() == 0) {
		petri::iterator b = dst.create(hse::place());

		result.source = petri::bound({{b}});
		result.sink = petri::bound({{b}});

		if (syntax.reset >= 0) {
			result.reset = result.source;
		}
	}

	return result;
}

petri::segment import_segment(hse::graph &dst, const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define) {
	if (syntax.region != "") {
		default_id = atoi(syntax.region.c_str());
	}

	petri::segment result;
	if (syntax.branches.empty()) {
		return result;
	}

	for (int i = 0; i < (int)syntax.branches.size(); i++) {
		petri::segment branch;
		if (syntax.branches[i].first.valid and import_cover(syntax.branches[i].first, dst, tokens, default_id, auto_define) != 1) {
			branch = dst.compose(petri::sequence, branch, import_segment(dst, syntax.branches[i].first, syntax.assume, default_id, tokens, auto_define).nodes);
		}
		if (syntax.branches[i].second.valid) {
			branch = dst.compose(petri::sequence, branch, import_segment(dst, syntax.branches[i].second, default_id, tokens, auto_define));
		}

		result = dst.compose(petri::choice, result, branch);
	}

	if (result.source.size() > 1u or syntax.repeat) {
		petri::iterator p = dst.create(hse::place());
		dst.connect({{p}}, result.source);
		result.source = petri::bound({{p}});
	}

	if (result.sink.size() > 1u) {
		petri::iterator p = dst.create(hse::place());
		dst.connect(result.sink, {{p}});
		result.sink = petri::bound({{p}});
	}

	if (not syntax.deterministic) {
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (auto j = i->begin(); j != i->end(); j++) {
				if (not syntax.stable) {
					dst.places[j->index].synchronizer = true;
				} else {
					dst.places[j->index].arbiter = true;
				}
			}
		}
	}

	if (syntax.repeat) {
		dst.connect(result.sink, result.source);
		result.sink.clear();

		boolean::cover repeat = 1;
		for (int i = 0; i < (int)syntax.branches.size() and not repeat.is_null(); i++) {
			if (syntax.branches[i].first.valid) {
				if (i == 0) {
					repeat = ~import_cover(syntax.branches[i].first, dst, tokens, default_id, auto_define);
				} else {
					repeat &= ~import_cover(syntax.branches[i].first, dst, tokens, default_id, auto_define);
				}
			} else {
				repeat = 0;
				break;
			}
		}

		if (not repeat.is_null()) {
			hse::iterator guard = dst.create(hse::transition(1, repeat));
			dst.connect(result.source, petri::bound({{guard}}));
			hse::iterator arrow = dst.create(hse::place());
			dst.connect(guard, arrow);

			result.sink = petri::bound({{arrow}});
		}

		if (not result.reset.empty()) {
			result.source = result.reset;
			result.reset.clear();
		}
	}

	return result;
}

void import_hse(hse::graph &dst, const parse_chp::composition &syntax, tokenizer *tokens, bool auto_define) {
	petri::segment result = import_segment(dst, syntax, 0, tokens, auto_define);
	if (not result.reset.empty()) {
		result.source = result.reset;
		result.reset.clear();
	}

	if (result.source.size() > 1u) {
		petri::iterator p = dst.create(hse::place());
		dst.connect({{p}}, result.source);
		result.source = petri::bound({{p}});
	} else {
		for (auto i = result.source.begin(); i != result.source.end(); i++) {
			for (int j = (int)i->size()-1; j >= 0; j--) {
				if (i->nodes[j].type == hse::transition::type) {
					vector<petri::iterator> p = dst.prev(i->nodes[j]);
					if (p.empty()) {
						p.push_back(dst.create(hse::place()));
						dst.connect(p.back(), i->nodes[j]);
					}
					i->nodes.erase(i->nodes.begin()+j);
					i->nodes.insert(i->nodes.end(), p.begin(), p.end());
				}
			}
		}
	}

	vector<hse::state> reset;
	for (auto i = result.source.begin(); i != result.source.end(); i++) {
		hse::state rst;
		for (auto j = i->begin(); j != i->end(); j++) {
			rst.tokens.push_back(petri::token(j->index));
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
