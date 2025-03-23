#pragma once

#include <hse/graph.h>

#include <parse_expression/expression.h>
#include <parse_expression/assignment.h>

namespace hse {

hse::graph import_hse(const parse_expression::expression &syntax, bool assume, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_hse(const parse_expression::assignment &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
