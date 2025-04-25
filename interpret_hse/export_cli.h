#pragma once

#include <hse/encoder.h>

namespace hse {

/*parse_hse::parallel export_parallel(const hse::graph &g);*/
string export_node(petri::iterator i, const hse::graph &g);
void print_conflicts(const hse::encoder &enc);

}

