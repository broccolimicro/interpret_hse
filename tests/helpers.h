#pragma once

#include <hse/graph.h>

namespace test {

// Helper function to find transitions by their action
vector<petri::iterator> find_transitions(const hse::graph &g, const boolean::cover &action);

// Helper function to check if two transitions are sequenced
bool are_sequenced(const hse::graph &g, petri::iterator a, petri::iterator b);

}

