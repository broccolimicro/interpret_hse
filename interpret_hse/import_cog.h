#pragma once

#include <hse/graph.h>

#include <parse_cog/composition.h>
#include <parse_cog/control.h>

namespace hse {

hse::graph import_hse(const parse_cog::composition &syntax, boolean::cover &covered, bool &hasRepeat, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_hse(const parse_cog::control &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
