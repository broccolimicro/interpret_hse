#pragma once

#include <hse/graph.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace hse {

petri::segment import_segment(hse::graph &dst, const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define);
petri::segment import_segment(hse::graph &dst, const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
