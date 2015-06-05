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

#include <parse_boolean/assignment.h>
#include <parse_boolean/guard.h>

#ifndef interpret_hse_import_h
#define interpret_hse_import_h

hse::graph import_graph(const parse_boolean::guard &syntax, boolean::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_boolean::assignment &syntax, boolean::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_hse::sequence &syntax, boolean::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_hse::parallel &syntax, boolean::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_hse::condition &syntax, boolean::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);
hse::graph import_graph(const parse_hse::loop &syntax, boolean::variable_set &variables, int default_id, tokenizer *tokens, bool auto_define);

#endif
