#pragma once

#include <common/standard.h>

#include <hse/graph.h>

#include <parse_astg/node.h>
#include <parse_astg/arc.h>
#include <parse_astg/graph.h>

namespace hse {

hse::iterator import_hse(hse::graph &dst, const parse_astg::node &syntax, tokenizer *token);
void import_hse(hse::graph &dst, const parse_astg::arc &syntax, tokenizer *tokens);
void import_hse(hse::graph &dst, const parse_astg::graph &syntax, tokenizer *tokens);

}
