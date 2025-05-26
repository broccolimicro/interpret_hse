#pragma once

#include <hse/graph.h>

#include <parse_cog/composition.h>
#include <parse_cog/control.h>

namespace hse {

petri::segment import_segment(hse::graph &dst, const parse_cog::composition &syntax, boolean::cover &covered, bool &hasRepeat, int default_id, tokenizer *tokens, bool auto_define);
petri::segment import_segment(hse::graph &dst, const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define);
void import_hse(hse::graph &dst, const parse_cog::composition &syntax, tokenizer *tokens, bool auto_define);

}
