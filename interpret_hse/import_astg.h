#pragma once

#include <common/standard.h>

#include <hse/graph.h>

#include <parse_astg/node.h>
#include <parse_astg/arc.h>
#include <parse_astg/graph.h>

namespace hse {

hse::iterator import_hse(const parse_astg::node &syntax, hse::graph &g, tokenizer *token);
void import_hse(const parse_astg::arc &syntax, hse::graph &g, tokenizer *tokens);
hse::graph import_hse(const parse_astg::graph &syntax, tokenizer *tokens);

}
