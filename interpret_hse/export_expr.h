#pragma once

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

#include <hse/graph.h>

#include <interpret_boolean/interface.h>

namespace hse {

parse_chp::composition export_parallel(boolean::cube c, boolean::ConstNetlist nets);
parse_chp::control export_control(boolean::cover c, boolean::ConstNetlist nets);

}
