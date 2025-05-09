#pragma once

#include <parse_astg/graph.h>
#include <hse/graph.h>

namespace hse {

pair<parse_astg::node, parse_astg::node> export_astg(parse_astg::graph &g, parse_expression::composition c, string label);
parse_astg::graph export_astg(const hse::graph &g);
void export_astg(string path, const hse::graph &g);

}
