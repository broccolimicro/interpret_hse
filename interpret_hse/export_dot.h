#pragma once


#include <parse_dot/node_id.h>
#include <parse_dot/attribute_list.h>
#include <parse_dot/statement.h>
#include <parse_dot/graph.h>

#include <hse/graph.h>

namespace hse {

parse_dot::node_id export_node_id(const petri::iterator &i);
parse_dot::attribute_list export_attribute_list(const hse::iterator i, const hse::graph &g, bool labels = false, bool notations = false, bool ghost = false, int encodings = -1);
parse_dot::statement export_statement(const hse::iterator &i, const hse::graph &g, bool labels = false, bool notations = false, bool ghost = false, int encodings = -1);
parse_dot::statement export_statement(const pair<int, int> &a, const hse::graph &g, bool labels = false);
parse_dot::graph export_graph(const hse::graph &g, bool horiz = false, bool labels = false, bool notations = false, bool ghost = false, int encodings = -1);

}
