#include "import_expr.h"

#include <common/standard.h>
#include <petri/iterator.h>

namespace hse {

segment::segment(bool cond) {
	this->loop = false;
	this->cond = cond;
}

segment::~segment() {
}

segment compose(hse::graph &dst, petri::Composition composition, segment s0, segment s1) {
	if (composition == petri::CHOICE) {
		s0.cond = s0.cond | s1.cond;
	} else if (composition == petri::PARALLEL) {
		s0.cond = s0.cond & s1.cond;
	} else if (composition == petri::SEQUENCE) {
		if (s0.nodes.source.empty()) {
			s0.cond = s1.cond;
			s0.loop = s1.loop;
		}
	}
	s0.nodes = dst.compose(composition, s0.nodes, s1.nodes);
	return s0;
}

}
