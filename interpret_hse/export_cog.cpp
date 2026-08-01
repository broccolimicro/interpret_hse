#include "export_cog.h"

#include <interpret_boolean/export.h>
#include <parse_cog/expression.h>

namespace parse_cog {

BooleanExpressionExporter::BooleanExpressionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

BooleanExpressionExporter::~BooleanExpressionExporter() {
}

parse_expression::operation BooleanExpressionExporter::export_operator(int func) const {
	using operation = parse_expression::operation;

	switch (func) {
	case NOT: return operation("~", "", "", "");
	case INTERFERE: return operation("?", "", "", "");
	case AND: return operation("", "", "&", "");
	case OR: return operation("", "", "|", "");
	}
	return operation();
}

const parse_expression::precedence_set &BooleanExpressionExporter::precedence() const {
	return expression_config::cfg->order;
}

parse_expression::expression::argument BooleanExpressionExporter::export_constant(int value) const {
	constant result;
	result.value = boolean::export_value(value);
	return {0, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression::argument BooleanExpressionExporter::export_literal(size_t index) const {
	literal result;
	result.name = nets.netAt(index);
	return {1, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression export_expression(boolean::cube expr, ucs::ConstNetlist nets) {
	return BooleanExpressionExporter(nets).export_expression(expr);
}

parse_expression::expression export_expression(boolean::cover expr, ucs::ConstNetlist nets) {
	return BooleanExpressionExporter(nets).export_expression(expr);
}

parse_expression::expression export_expression_xfactor(boolean::cover expr, ucs::ConstNetlist nets) {
	return BooleanExpressionExporter(nets).export_expression_xfactor(expr);
}

parse_expression::expression export_expression_hfactor(boolean::cover expr, ucs::ConstNetlist nets) {
	return BooleanExpressionExporter(nets).export_expression_hfactor(expr);
}

BooleanCompositionExporter::BooleanCompositionExporter(ucs::ConstNetlist nets) : nets(nets) {
}

BooleanCompositionExporter::~BooleanCompositionExporter() {
}

parse_expression::operation BooleanCompositionExporter::export_operator(int func) const {
	using operation = parse_expression::operation;

	switch (func) {
	case AND: return operation("", "", ",", "");
	case OR: return operation("", "", ":", "");
	}
	return operation();
}

const parse_expression::precedence_set &BooleanCompositionExporter::precedence() const {
	return composition_config::cfg->order;
}

parse_expression::expression::argument BooleanCompositionExporter::export_constant(int value) const {
	assignment result;
	result.valid = true;
	return {1, std::shared_ptr<parse::syntax>(result.clone())};
}

parse_expression::expression::argument BooleanCompositionExporter::export_literal(size_t index) const {
	literal result;
	result.name = nets.netAt(index);
	return {1, std::shared_ptr<parse::syntax>(result.clone())};
}

assignment BooleanCompositionExporter::export_assignment(size_t index, int value) const {
	assignment result;
	result.valid = true;
	if (value >= 2) {
		return result;
	}

	parse_expression::expression lvalue;
	lvalue.valid = true;
	lvalue.level = 0;
	lvalue.type = expression_config::cfg->order.type(0);
	lvalue.arguments.push_back(export_literal(index));
	result.left.push_back(lvalue);

	if (value == 0) {
		result.operation = "-";
	} else if (value == 1) {
		result.operation = "+";
	} else {
		result.operation = "~";
	}

	return result;
}

parse_expression::expression::argument BooleanCompositionExporter::export_term(size_t index, int value) const {
	return {1, std::shared_ptr<parse::syntax>(export_assignment(index, value).clone())};
}

assignment export_assignment(size_t index, int value, ucs::ConstNetlist nets) {
	return BooleanCompositionExporter(nets).export_assignment(index, value);
}

parse_expression::expression export_composition(boolean::cube expr, ucs::ConstNetlist nets) {
	return BooleanCompositionExporter(nets).export_expression(expr);
}

parse_expression::expression export_composition(boolean::cover expr, ucs::ConstNetlist nets) {
	return BooleanCompositionExporter(nets).export_expression(expr);
}

parse_expression::expression export_composition_xfactor(boolean::cover expr, ucs::ConstNetlist nets) {
	return BooleanCompositionExporter(nets).export_expression_xfactor(expr);
}

parse_expression::expression export_composition_hfactor(boolean::cover expr, ucs::ConstNetlist nets) {
	return BooleanCompositionExporter(nets).export_expression_hfactor(expr);
}


parse_cog::composition export_parallel(boolean::cube c, ucs::ConstNetlist nets) {
	parse_cog::composition result;
	result.valid = true;

	result.level = parse_cog::composition::PARALLEL;
	for (int i = 0; i < (int)c.values.size()*16; i++) {
		int val = c.get(i);
		if (val == 2) {
			continue;
		}
		if (not result.branches.empty()) {
			result.comp.push_back("and");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_assignment(i, val, nets).clone()));
	}

	return result;
}

parse_cog::composition export_choice(boolean::cover c, ucs::ConstNetlist nets) {
	parse_cog::composition result;
	result.valid = true;

	result.level = parse_cog::composition::CHOICE;
	for (int i = 0; i < (int)c.cubes.size(); i++) {
		if (i != 0) {
			result.comp.push_back("xor");
		}
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_parallel(c.cubes[i], nets).clone()));
	}

	return result;
}

parse_cog::composition export_sequence(vector<petri::iterator> &i, const hse::graph &g) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::INTERNAL_SEQUENCE;

	vector<petri::iterator> covered;
	while (true) {
		if (i.size() == 1 and i[0].type == hse::transition::type) {
			if (not g.transitions[i[0].index].guard.is_tautology()) {
				parse_cog::control c;
				c.valid = true;
				c.kind = "await";
				c.guard = export_expression(g.transitions[i[0].index].guard, g);
				result.branches.push_back(std::shared_ptr<parse::syntax>(c.clone()));
			}

			if (g.transitions[i[0].index].local_action.cubes.size() == 1) {
				vector<int> vars = g.transitions[i[0].index].local_action.cubes[0].vars();
				if (vars.size() == 1) {
					result.branches.push_back(std::shared_ptr<parse::syntax>(export_assignment(vars[0], g.transitions[i[0].index].local_action.cubes[0].get(vars[0]), g).clone()));
				} else {
					result.branches.push_back(std::shared_ptr<parse::syntax>(export_parallel(g.transitions[i[0].index].local_action.cubes[0], g).clone()));
				}
			} else {
				result.branches.push_back(std::shared_ptr<parse::syntax>(export_choice(g.transitions[i[0].index].local_action, g).clone()));
			}
		} else if (i.size() > 1 and i[0].type == hse::place::type) {
			result.branches.push_back(std::shared_ptr<parse::syntax>(export_parallel(i, g).clone()));
		}

		vector<petri::iterator> n = g.next(i);
		sort(n.begin(), n.end());
		n.resize(unique(n.begin(), n.end()) - n.begin());

		vector<petri::iterator> p = g.prev(n);
		sort(p.begin(), p.end());
		p.resize(unique(p.begin(), p.end()) - p.begin());

		if (vector_intersection_size(covered, p) != 0 || p.size() > i.size()) {
			for (int i = 1; i < (int)result.branches.size(); i++) {
				result.comp.push_back(";");
			}
			return result;
		} else {
			covered.insert(covered.end(), i.begin(), i.end());
			i = n;
		}
	}
}

parse_cog::composition export_parallel(vector<petri::iterator> &i, const hse::graph &g) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::PARALLEL;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++) {
		if (j != 0) {
			result.comp.push_back("or");
		}
		vector<petri::iterator> start(1, i[j]);
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_sequence(start, g).clone()));
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}

parse_cog::composition export_choice(vector<petri::iterator> &i, const hse::graph &g) {
	parse_cog::composition result;
	result.valid = true;
	result.level = parse_cog::composition::CONDITION;
	vector<petri::iterator> end;

	for (int j = 0; j < (int)i.size(); j++) {
		vector<petri::iterator> start(1, i[j]);
		parse_cog::composition s = export_sequence(start, g);
		result.branches.push_back(std::shared_ptr<parse::syntax>(export_sequence(start, g).clone()));
		end.insert(end.end(), start.begin(), start.end());
	}

	i = end;

	return result;
}


/*parse_cog::composition export_sequence(vector<petri::iterator> nodes, map<petri::iterator, int> counts, const hse::graph &g, const ucs::variable_set &v)
{
	// Maintain a stack to help us manage the hierarchy. The deeper
	// the stack, the more hierarchy there is. This stack stores
	// only sequences. Since we know at this point that every parallel
	// or conditional block we introduce will only have one branch, we
	// can get away with this. We will merge these sequences later to
	// add parallelism or choice.
	parse_cog::composition head;
	head.valid = true;
	head.level = 1;
	vector<parse_cog::composition*> stack;
	stack.push_back(&head);

	int delta = 0;
	int value = counts[nodes[0]]-1;
	petri::iterator last = nodes[0];
	for (int j = 1; j < (int)nodes.size(); j++)
	{
		int c = counts[nodes[j]]-1;
		delta = c - value;
		value = c;

		// The count increased, we need to remove hierarchy
		if (delta > 0 && j > 1)
		{
			if (stack.size() == 0)
			{
				error("", "hse not properly nested", __FILE__, __LINE__);
				return parse_cog::composition();
			}

			stack.back()->end = last.index;

			if (stack.size() > 1)
			{
				stack[stack.size()-2]->branches.back().assign.end = nodes[j].index;
				stack[stack.size()-2]->branches.back().ctrl.end = nodes[j].index;
				stack[stack.size()-2]->branches.back().sub.end = nodes[j].index;
			}

			stack.pop_back();
			delta--;
		}

		// The count decreased, we need to add hierarchy
		if (delta < 0)
		{
			// The last node before the count decrease was a transition, meaning
			// we need to wrap the next couple transitions in a parallel block
			if (last.type == hse::transition::type)
			{
				parse_cog::composition tmp;
				tmp.valid = true;
				tmp.level = 0;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp.start = last.index;

				parse_cog::composition new_head;
				new_head.valid = true;
				new_head.level = 1;
				new_head.start = nodes[j].index;
				tmp.branches.push_back(parse_cog::branch(new_head));

				stack.back()->branches.push_back(parse_cog::branch(tmp));
				stack.push_back(&stack.back()->branches.back().sub.branches.back().sub);
			}
			// The last node before the count decrease was a place, meaning we need
			// to wrap the next couple transitions in a conditional block. Never
			// mind the guard, we will take care of that later.
			else if (last.type == hse::place::type)
			{
				parse_cog::control tmp;
				tmp.valid = true;

				// This is very important: we need to keep track of what
				// syntaxes belong to what nodes. That way we can compare them
				// later when we go to merge them.
				tmp.start = last.index;

				tmp.branches.back().second.valid = true;
				parse_cog::composition new_head;
				new_head.level = 1;
				new_head.valid = true;
				new_head.start = nodes[j].index;
				tmp.branches.push_back(pair<parse_cog::expression, parse_cog::composition>(parse_cog::expression(), new_head));

				stack.back()->branches.push_back(parse_cog::branch(tmp));
				stack.push_back(&stack.back()->branches.back().ctrl.branches.back().second);
			}

			delta++;
		}

		// Once we have dealt with the hierarchy issues, we still need to add assignments and disjunctions into the hse block.
		// We'll package the disjunctions into conditionals later.
		if (stack.size() == 0)
		{
			internal("", "empty stack", __FILE__, __LINE__);
			return head;
		}
		else if (nodes[j].type == hse::transition::type && g.transitions[nodes[j].index].behavior == hse::transition::active)
		{
			if (g.transitions[nodes[j].index].local_action.cubes.size() == 1)
			{
				vector<int> vars = g.transitions[nodes[j].index].local_action.cubes[0].vars();
				if (vars.size() == 1)
					stack.back()->branches.push_back(parse_cog::branch(export_assignment<parse_cog::assignment>(vars[0], g.transitions[nodes[j].index].local_action.cubes[0].get(vars[0]), v)));
				else
					stack.back()->branches.push_back(parse_cog::branch(export_composition(g.transitions[nodes[j].index].local_action, v)));
			}
			else
				stack.back()->branches.push_back(parse_cog::branch(export_control(g.transitions[nodes[j].index].local_action, v)));

			stack.back()->branches.back().ctrl.start = nodes[j].index;
			stack.back()->branches.back().ctrl.end = nodes[j].index;
			stack.back()->branches.back().sub.start = nodes[j].index;
			stack.back()->branches.back().sub.end = nodes[j].index;
			stack.back()->branches.back().assign.start = nodes[j].index;
			stack.back()->branches.back().assign.end = nodes[j].index;
		}
		else if (nodes[j].type == hse::transition::type && g.transitions[nodes[j].index].behavior == hse::transition::passive)
		{
			stack.back()->branches.push_back(parse_cog::branch(parse_cog::control()));
			stack.back()->branches.back().ctrl.branches.push_back(pair<parse_cog::expression, parse_cog::composition>(export_expression_xfactor(g.transitions[nodes[j].index].local_action, v), parse_cog::composition()));

			stack.back()->branches.back().ctrl.start = nodes[j].index;
			stack.back()->branches.back().ctrl.end = nodes[j].index;
			stack.back()->branches.back().sub.start = nodes[j].index;
			stack.back()->branches.back().sub.end = nodes[j].index;
			stack.back()->branches.back().assign.start = nodes[j].index;
			stack.back()->branches.back().assign.end = nodes[j].index;
		}

		last = nodes[j];
	}

	return head;
}

// TODO this doesn't handle the case where one sequence is a subset of another.
bool merge_sequences(parse_cog::composition &s0, parse_cog::composition &s1, vector<parse::syntax*> &m)
{
	int offset = 0;
	bool equal = false;
	if (s0.branches.size() >= s1.branches.size())
	{
		for (offset = 0; offset < (int)s0.branches.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s1.branches.size() && equal; k++)
				if (s0.branches[(offset + k)%s0.branches.size()].sub.start != s1.branches[k].sub.start)
					equal = false;
		}
	}
	else if (s1.branches.size() > s0.branches.size())
	{
		for (offset = 0; offset < (int)s1.branches.size() && !equal; offset++)
		{
			equal = true;
			for (int k = 0; k < (int)s0.branches.size() && equal; k++)
				if (s1.branches[(offset + k)%s1.branches.size()].sub.start != s0.branches[k].sub.start)
					equal = false;
		}

		if (equal)
			swap(s0, s1);
	}

	offset--;

	for (int k = 0; k < (int)s1.branches.size() && equal; k++)
	{
		if (s0.branches[(offset+k)%s0.branches.size()]->is_a<parse_hse::parallel>() &&
			s1.branches[k]->is_a<parse_hse::parallel>())
		{
			parse_hse::parallel *tmp0 = (parse_hse::parallel *)s0.branches[(offset+k)%s0.branches.size()];
			parse_hse::parallel *tmp1 = (parse_hse::parallel *)s1.branches[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
		else if (s0.branches[(offset+k)%s0.branches.size()]->is_a<parse_hse::condition>() &&
				s1.branches[k]->is_a<parse_hse::condition>())
		{
			parse_hse::condition *tmp0 = (parse_hse::condition *)s0.branches[(offset+k)%s0.branches.size()];
			parse_hse::condition *tmp1 = (parse_hse::condition *)s1.branches[k];
			tmp0->branches.insert(tmp0->branches.end(), tmp1->branches.begin(), tmp1->branches.end());
			m.push_back(tmp0);
		}
	}

	return equal;
}*/

}
