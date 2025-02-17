#pragma once

#include <common/standard.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

#include <parse_dot/node_id.h>
#include <parse_dot/attribute_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <parse_astg/graph.h>

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

#include <hse/graph.h>
#include <hse/state.h>
#include <hse/simulator.h>

#include <interpret_boolean/export.h>

namespace hse {

// ASTG

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &g, parse_expression::composition c, ucs::variable_set &variables, string label);
parse_astg::graph export_astg(const hse::graph &g, ucs::variable_set &variables);
void export_astg(string path, const hse::graph &g, ucs::variable_set &variables);

// DOT

parse_dot::node_id export_node_id(const petri::iterator &i);
parse_dot::attribute_list export_attribute_list(const hse::iterator i, const hse::graph &g, ucs::variable_set &variables, bool labels = false, bool notations = false, bool ghost = false, int encodings = -1);
parse_dot::statement export_statement(const hse::iterator &i, const hse::graph &g, ucs::variable_set &v, bool labels = false, bool notations = false, bool ghost = false, int encodings = -1);
parse_dot::statement export_statement(const pair<int, int> &a, const hse::graph &g, ucs::variable_set &v, bool labels = false);
parse_dot::graph export_graph(const hse::graph &g, ucs::variable_set &v, bool horiz = false, bool labels = false, bool notations = false, bool ghost = false, int encodings = -1);

// HSE

parse_chp::composition export_parallel(boolean::cube c, ucs::variable_set &variables);
parse_chp::control export_control(boolean::cover c, ucs::variable_set &variables);
parse_chp::composition export_sequence(vector<petri::iterator> &i, const hse::graph &g, ucs::variable_set &v);
parse_chp::composition export_parallel(vector<petri::iterator> &i, const hse::graph &g, ucs::variable_set &v);
parse_chp::control export_control(vector<petri::iterator> &i, const hse::graph &g, ucs::variable_set &v);

/*parse_chp::composition export_sequence(vector<hse::iterator> &i, const hse::graph &g, ucs::variable_set &v);
parse_chp::composition export_parallel(vector<hse::iterator> &i, const hse::graph &g, ucs::variable_set &v);
parse_chp::control export_control(vector<hse::iterator> &i, const hse::graph &g, ucs::variable_set &v);*/

/*parse_hse::parallel export_parallel(const hse::graph &g, const boolean::variable_set &v);*/
string export_node(petri::iterator i, const hse::graph &g, const ucs::variable_set &v);

}
