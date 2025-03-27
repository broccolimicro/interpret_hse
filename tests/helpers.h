#pragma once

#include <hse/graph.h>

namespace test {

vector<petri::iterator> findRule(const hse::graph &g, const boolean::cover &guard, const boolean::cover &action);

}

