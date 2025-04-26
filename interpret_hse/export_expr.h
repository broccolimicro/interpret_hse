#pragma once

#include <parse_chp/composition.h>
#include <parse_chp/control.h>

#include <hse/graph.h>

#include <common/net.h>

namespace hse {

parse_chp::composition export_parallel(boolean::cube c, ucs::ConstNetlist nets);
parse_chp::control export_control(boolean::cover c, ucs::ConstNetlist nets);

}
