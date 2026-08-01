#pragma once

#include <common/standard.h>

#include <hse/graph.h>

#include <parse_astg/node.h>
#include <parse_astg/arc.h>
#include <parse_astg/graph.h>

#include <parse_expression/import.h>

namespace parse_astg {

struct BooleanExpressionImporter : parse_expression::LValuedImporter<boolean::cover, std::string> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	BooleanExpressionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~BooleanExpressionImporter();

	boolean::cover L_to_T(std::string lval, tokenizer *tokens) const override;
	std::string T_to_L(boolean::cover expr, tokenizer *tokens) const override;

	bool is_lvalue(const parse_expression::expression &syntax) const override;
	
	std::string import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;

	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;

	std::string import_modifier(parse_expression::operation op, vector<std::string> args, tokenizer *tokens) const override;

	boolean::cover import_unary(parse_expression::operation op, boolean::cover expr, tokenizer *tokens) const override;
	boolean::cover import_binary(parse_expression::operation op, boolean::cover left, boolean::cover right, tokenizer *tokens) const override;
	boolean::cover import_modifier(parse_expression::operation op, vector<boolean::cover> args, tokenizer *tokens) const override;
};

boolean::cover import_cover(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);
boolean::cube import_cube(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);

struct BooleanCompositionImporter : parse_expression::Importer<boolean::cover> {
	ucs::Netlist symbols;
	vector<int> region;
	bool autoDefine;

	BooleanCompositionImporter(ucs::Netlist symbols, int region = 0, bool autoDefine = false);
	~BooleanCompositionImporter();

	boolean::cube import_assignment(const assignment &syntax, tokenizer *tokens) const;
	boolean::cover import_term(const parse_expression::expression::argument &syntax, tokenizer *tokens) const override;
	void push_properties(parse_expression::operation op, const vector<parse_expression::expression::argument> &args, tokenizer *tokens) override;
	void pop_properties(parse_expression::operation op) override;
	boolean::cover import_binary(parse_expression::operation op, boolean::cover left, boolean::cover right, tokenizer *tokens) const override;
	boolean::cover import_modifier(parse_expression::operation op, vector<boolean::cover> args, tokenizer *tokens) const override;
};

boolean::cube import_boolean_assignment(const assignment &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);
boolean::cover import_boolean_choice(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);
boolean::cube import_boolean_parallel(const parse_expression::expression &syntax, ucs::Netlist nets, tokenizer *tokens, int region = 0, bool auto_define = false);

}

namespace hse {

hse::iterator import_hse(hse::graph &dst, const parse_astg::node &syntax, tokenizer *token);
void import_hse(hse::graph &dst, const parse_astg::arc &syntax, tokenizer *tokens);
void import_hse(hse::graph &dst, const parse_astg::graph &syntax, tokenizer *tokens);

}
