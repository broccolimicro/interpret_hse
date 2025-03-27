#include "helpers.h"
#include <common/standard.h>

namespace test {

vector<petri::iterator> findRule(const hse::graph &g, const boolean::cover &guard, const boolean::cover &action) {
	vector<petri::iterator> result;
	for (int i = 0; i < (int)g.transitions.size(); i++) {
		if (g.transitions[i].guard == guard and g.transitions[i].local_action == action) {
			result.push_back(petri::iterator(petri::transition::type, i));
		}
	}
	return result;
}

}
