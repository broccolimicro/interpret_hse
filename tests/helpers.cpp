#include "helpers.h"

namespace test {

vector<petri::iterator> find_transitions(const hse::graph &g, const boolean::cover &action) {
	vector<petri::iterator> result;
	// Find transitions by action
	for (int i = 0; i < (int)g.transitions.size(); i++) {
		if (g.transitions[i].local_action == action) {
			result.push_back(petri::iterator(petri::transition::type, i));
		}
	}
	return result;
}

bool are_sequenced(const hse::graph &g, petri::iterator a, petri::iterator b) {
	vector<petri::iterator> ao = g.next(a);
	vector<petri::iterator> bi = g.prev(b);
	sort(ao.begin(), ao.end());
	sort(bi.begin(), bi.end());
	return vector_intersects(ao, bi);
}

}
