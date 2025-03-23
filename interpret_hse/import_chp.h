#pragma once

#include <hse/graph.h>

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

namespace hse {

hse::graph import_hse(const parse_chp::composition &syntax, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_hse(const parse_chp::control &syntax, int default_id, tokenizer *tokens, bool auto_define);

}
