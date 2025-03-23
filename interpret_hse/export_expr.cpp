#include "export_expr.h"

#include <interpret_boolean/export.h>

namespace hse {

parse_chp::composition export_parallel(boolean::cube c, boolean::ConstNetlist nets)
{
	parse_chp::composition result;
	result.valid = true;

	result.level = 2;//parse_expression::composition::get_level(",");

	for (int i = 0; i < (int)c.values.size()*16; i++) {
		int val = c.get(i);
		if (val != 2) {
			result.branches.push_back(boolean::export_assignment(i, val, nets));
		}
	}

	return result;
}

parse_chp::control export_control(boolean::cover c, boolean::ConstNetlist nets)
{
	parse_chp::control result;
	result.valid = true;

	result.deterministic = false;

	for (int i = 0; i < (int)c.cubes.size(); i++) {
		result.branches.push_back(pair<parse_expression::expression, parse_chp::composition>(parse_expression::expression(), export_parallel(c.cubes[i], nets)));
	}

	return result;
}

}
