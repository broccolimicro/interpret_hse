#pragma once

#include <hse/graph.h>

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

namespace hse {

petri::segment import_segment(hse::graph &dst, const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define);
petri::segment import_segment(hse::graph &dst, const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define);
void import_hse(hse::graph &dst, const parse_chp::composition &syntax, tokenizer *tokens, bool auto_define);

}
