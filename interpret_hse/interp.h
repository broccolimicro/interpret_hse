/*
 * interp.h
 *
 *  Created on: Feb 5, 2015
 *      Author: nbingham
 */

#include <hse/graph.h>
#include <parse_hse/parallel.h>

#ifndef interpret_hse_interp_h
#define interpret_hse_interp_h

hse::graph import_hse(parse_hse::parallel syntax);
parse_hse::parallel export_hse(hse::graph circuit);

#endif
