/*
 * export.h
 *
 *  Created on: Feb 6, 2015
 *      Author: nbingham
 */

#include <common/standard.h>

#include <boolean/variable.h>
#include <boolean/cube.h>
#include <boolean/cover.h>

#include <parse_boolean/variable_name.h>
#include <parse_boolean/internal_choice.h>
#include <parse_boolean/disjunction.h>

#include <parse_hse/sequence.h>
#include <parse_hse/parallel.h>
#include <parse_hse/condition.h>
#include <parse_hse/loop.h>

#include <hse/graph.h>
#include <hse/simulator.h>

#include <interpret_boolean/export.h>

#ifndef interpret_hse_export_h
#define interpret_hse_export_h

/*parse_hse::sequence export_sequence(vector<hse::iterator> &i, const hse::graph &g, boolean::variable_set &v);
parse_hse::parallel export_parallel(vector<hse::iterator> &i, const hse::graph &g, boolean::variable_set &v);
parse::syntax *export_condition(vector<hse::iterator> &i, const hse::graph &g, boolean::variable_set &v);*/
string export_token(boolean::variable_set &v, hse::token t);
string export_state(boolean::variable_set &v, const hse::simulator &s);
string export_instability(const hse::graph &g, boolean::variable_set &v, hse::instability i);
string export_interference(const hse::graph &g, boolean::variable_set &v, hse::interference i);
string export_deadlock(boolean::variable_set &v, hse::deadlock d);
parse_hse::parallel export_parallel(const hse::graph &g, boolean::variable_set &v);

#endif
