/*
 * import.h
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include <common/standard.h>

#include <ucs/variable.h>
#include <hse/graph.h>
#include <hse/state.h>

#include <parse_astg/node.h>
#include <parse_astg/arc.h>
#include <parse_astg/graph.h>

#include <parse_dot/node_id.h>
#include <parse_dot/assignment.h>
#include <parse_dot/assignment_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

#ifndef interpret_hse_import_h
#define interpret_hse_import_h

// ASTG

hse::iterator import_graph(const parse_astg::node &syntax, ucs::variable_set &variables, hse::graph &g, tokenizer *token);
void import_graph(const parse_astg::arc &syntax, ucs::variable_set &variables, hse::graph &g, tokenizer *tokens);
hse::graph import_graph(const parse_astg::graph &syntax, ucs::variable_set &variables, tokenizer *tokens);

// DOT

hse::iterator import_graph(const parse_dot::node_id &syntax, map<string, hse::iterator> &nodes, ucs::variable_set &variables, hse::graph &g, tokenizer *token, bool define, bool squash_errors);
map<string, string> import_graph(const parse_dot::attribute_list &syntax, tokenizer *tokens);
void import_graph(const parse_dot::statement &syntax, hse::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define);
void import_graph(const parse_dot::graph &syntax, hse::graph &g, ucs::variable_set &variables, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_dot::graph &syntax, ucs::variable_set &variables, tokenizer *tokens, bool auto_define);

// HSE

hse::graph import_graph(const parse_expression::expression &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_expression::assignment &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_chp::composition &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_chp::control &syntax, ucs::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);

#endif
