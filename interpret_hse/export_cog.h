#pragma once

#include <parse_cog/composition.h>
#include <parse_cog/control.h>

#include <hse/graph.h>
#include <common/net.h>

#include <interpret_boolean/export.h>

namespace parse_cog {

struct BooleanExpressionExporter : boolean::ExpressionExporter {
	ucs::ConstNetlist nets;

	BooleanExpressionExporter(ucs::ConstNetlist nets);
	~BooleanExpressionExporter();

	parse_expression::operation export_operator(int func) const override;
	const parse_expression::precedence_set &precedence() const override;

	parse_expression::expression::argument export_constant(int value) const override;
	parse_expression::expression::argument export_literal(size_t index) const override;
};

parse_expression::expression export_expression(boolean::cube expr, ucs::ConstNetlist nets);
parse_expression::expression export_expression(boolean::cover expr, ucs::ConstNetlist nets);
parse_expression::expression export_expression_xfactor(boolean::cover expr, ucs::ConstNetlist nets);
parse_expression::expression export_expression_hfactor(boolean::cover expr, ucs::ConstNetlist nets);

struct BooleanCompositionExporter : boolean::ExpressionExporter {
	ucs::ConstNetlist nets;

	BooleanCompositionExporter(ucs::ConstNetlist nets);
	~BooleanCompositionExporter();

	parse_expression::operation export_operator(int func) const override;
	const parse_expression::precedence_set &precedence() const override;

	parse_expression::expression::argument export_constant(int value) const override;
	parse_expression::expression::argument export_literal(size_t index) const override;
	assignment export_assignment(size_t index, int value) const;
	parse_expression::expression::argument export_term(size_t index, int value) const override;
};

assignment export_assignment(size_t index, int value, ucs::ConstNetlist nets);
parse_expression::expression export_composition(boolean::cube expr, ucs::ConstNetlist nets);
parse_expression::expression export_composition(boolean::cover expr, ucs::ConstNetlist nets);
parse_expression::expression export_composition_xfactor(boolean::cover expr, ucs::ConstNetlist nets);
parse_expression::expression export_composition_hfactor(boolean::cover expr, ucs::ConstNetlist nets);


parse_cog::composition export_parallel(boolean::cube c, ucs::ConstNetlist nets);
parse_cog::composition export_choice(boolean::cover c, ucs::ConstNetlist nets);
parse_cog::composition export_sequence(vector<petri::iterator> &i, const hse::graph &g);
parse_cog::composition export_parallel(vector<petri::iterator> &i, const hse::graph &g);
parse_cog::control export_control(vector<petri::iterator> &i, const hse::graph &g);

/*parse_cog::composition export_sequence(vector<hse::iterator> &i, const hse::graph &g);
parse_cog::composition export_parallel(vector<hse::iterator> &i, const hse::graph &g);
parse_cog::control export_control(vector<hse::iterator> &i, const hse::graph &g);*/

}
