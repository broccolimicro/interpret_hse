#pragma once

#include <hse/graph.h>

#include <parse_dot/node_id.h>
#include <parse_dot/assignment.h>
#include <parse_dot/assignment_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

namespace hse {

hse::iterator import_hse(const parse_dot::node_id &syntax, map<string, hse::iterator> &nodes, hse::graph &g, tokenizer *token, bool define, bool squash_errors);
map<string, string> import_hse(const parse_dot::attribute_list &syntax, tokenizer *tokens);
void import_hse(const parse_dot::statement &syntax, hse::graph &g, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define);
void import_hse(const parse_dot::graph &syntax, hse::graph &g, map<string, map<string, string> > &globals, map<string, hse::iterator> &nodes, tokenizer *tokens, bool auto_define);
hse::graph import_hse(const parse_dot::graph &syntax, tokenizer *tokens, bool auto_define);

}
