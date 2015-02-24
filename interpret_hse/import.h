/*
 * import.h
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include <common/standard.h>

#include <boolean/variable.h>
#include <hse/graph.h>

#include <parse_hse/sequence.h>
#include <parse_hse/parallel.h>
#include <parse_hse/condition.h>
#include <parse_hse/loop.h>

#include <parse_boolean/internal_choice.h>
#include <parse_boolean/disjunction.h>

#ifndef interpret_hse_import_h
#define interpret_hse_import_h

hse::graph import_graph(tokenizer &tokens, const parse_boolean::disjunction &syntax, boolean::variable_set &variables, bool auto_define);
hse::graph import_graph(tokenizer &tokens, const parse_boolean::internal_choice &syntax, boolean::variable_set &variables, bool auto_define);
hse::graph import_graph(tokenizer &tokens, const parse_hse::sequence &syntax, boolean::variable_set &variables, bool auto_define);
hse::graph import_graph(tokenizer &tokens, const parse_hse::parallel &syntax, boolean::variable_set &variables, bool auto_define);
hse::graph import_graph(tokenizer &tokens, const parse_hse::condition &syntax, boolean::variable_set &variables, bool auto_define);
hse::graph import_graph(tokenizer &tokens, const parse_hse::loop &syntax, boolean::variable_set &variables, bool auto_define);

#endif
