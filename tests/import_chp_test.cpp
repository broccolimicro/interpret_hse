#include <common/standard.h>
#include <gtest/gtest.h>

#include <hse/graph.h>
#include <parse/tokenizer.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse_chp/composition.h>
#include <interpret_hse/import_chp.h>

#include "helpers.h"

using namespace std;
using namespace test;

// Helper function to parse a string into an HSE graph
hse::graph load_hse_string(string input) {
	// Set up tokenizer
	tokenizer tokens;
	tokens.register_token<parse::block_comment>(false);
	tokens.register_token<parse::line_comment>(false);
	parse_chp::composition::register_syntax(tokens);

	tokens.insert("string_input", input, nullptr);

	hse::graph g;
	
	// Parse input
	tokens.increment(false);
	tokens.expect<parse_chp::composition>();
	if (tokens.decrement(__FILE__, __LINE__)) {
		parse_chp::composition syntax(tokens);
		g = hse::import_hse(syntax, 0, &tokens, true);
	}

	return g;
}

// Test basic sequence import (a+; b+; a-; b-)
TEST(ImportChp, Sequence) {
	hse::graph g = load_hse_string("a+; b+; a-; b-");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 4u);
	EXPECT_EQ(g.places.size(), 5u);

	int a = g.netIndex("a");
	int b = g.netIndex("b");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);

	vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
	vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
	vector<petri::iterator> a0 = find_transitions(g, boolean::cover(a, 0));
	vector<petri::iterator> b0 = find_transitions(g, boolean::cover(b, 0));
	
	EXPECT_FALSE(a1.empty());
	EXPECT_FALSE(b1.empty());
	EXPECT_FALSE(a0.empty());
	EXPECT_FALSE(b0.empty());

	EXPECT_TRUE(are_sequenced(g, a1[0], b1[0]));
	EXPECT_TRUE(are_sequenced(g, b1[0], a0[0]));
	EXPECT_TRUE(are_sequenced(g, a0[0], b0[0]));
}

// Test parallel composition import ((a+, b+); (a-, b-))
TEST(ImportChp, Parallel) {
	hse::graph g = load_hse_string("(a+, b+); (a-, b-)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 4u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);

	vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
	vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
	vector<petri::iterator> a0 = find_transitions(g, boolean::cover(a, 0));
	vector<petri::iterator> b0 = find_transitions(g, boolean::cover(b, 0));
	
	EXPECT_FALSE(a1.empty());
	EXPECT_FALSE(b1.empty());
	EXPECT_FALSE(a0.empty());
	EXPECT_FALSE(b0.empty());
	
	// Verify parallel structure - a+ and b+ should be concurrent
	EXPECT_FALSE(are_sequenced(g, a1[0], b1[0]));
	EXPECT_FALSE(are_sequenced(g, b1[0], a1[0]));
	
	// Verify a- and b- are concurrent
	EXPECT_FALSE(are_sequenced(g, a0[0], b0[0]));
	EXPECT_FALSE(are_sequenced(g, b0[0], a0[0]));
	
	// Verify sequencing - first parallel group completes before second group
	EXPECT_TRUE(are_sequenced(g, a1[0], a0[0]));
	EXPECT_TRUE(are_sequenced(g, b1[0], b0[0]));
}

// Test selection import ([c -> a+; a- [] ~c -> b+; b-])
TEST(ImportChp, Selection) {
	hse::graph g = load_hse_string("[c -> a+; a- [] ~c -> b+; b-]");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 3);  // a, b, and c
	EXPECT_EQ(g.transitions.size(), 4u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	EXPECT_GE(c, 0);

	vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
	vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
	vector<petri::iterator> a0 = find_transitions(g, boolean::cover(a, 0));
	vector<petri::iterator> b0 = find_transitions(g, boolean::cover(b, 0));
	
	EXPECT_FALSE(a1.empty());
	EXPECT_FALSE(b1.empty());
	EXPECT_FALSE(a0.empty());
	EXPECT_FALSE(b0.empty());
	
	// Verify selection structure (no path between a+ and b+)
	EXPECT_FALSE(are_sequenced(g, a1[0], b1[0]));
	EXPECT_FALSE(are_sequenced(g, b1[0], a1[0]));
	
	// Verify each branch internal sequencing
	EXPECT_TRUE(are_sequenced(g, a1[0], a0[0]));
	EXPECT_TRUE(are_sequenced(g, b1[0], b0[0]));
	
	// Verify guards on first transitions of each branch
	EXPECT_FALSE(g.transitions[a1[0].index].guard.is_tautology());
	EXPECT_FALSE(g.transitions[b1[0].index].guard.is_tautology());
}

// Test loop import (*[a+; b+; a-; b-])
TEST(ImportChp, Loop) {
	hse::graph g = load_hse_string("*[a+; b+; a-; b-]");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 4u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);

	vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
	vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
	vector<petri::iterator> a0 = find_transitions(g, boolean::cover(a, 0));
	vector<petri::iterator> b0 = find_transitions(g, boolean::cover(b, 0));
	
	EXPECT_FALSE(a1.empty());
	EXPECT_FALSE(b1.empty());
	EXPECT_FALSE(a0.empty());
	EXPECT_FALSE(b0.empty());
	
	// Verify cycle: should be able to go from any transition back to itself
	EXPECT_TRUE(are_sequenced(g, a1[0], a1[0]));
	EXPECT_TRUE(are_sequenced(g, b1[0], b1[0]));
	EXPECT_TRUE(are_sequenced(g, a0[0], a0[0]));
	EXPECT_TRUE(are_sequenced(g, b0[0], b0[0]));
	
	// Verify order within the loop
	EXPECT_TRUE(are_sequenced(g, a1[0], b1[0]));
	EXPECT_TRUE(are_sequenced(g, b1[0], a0[0]));
	EXPECT_TRUE(are_sequenced(g, a0[0], b0[0]));
	EXPECT_TRUE(are_sequenced(g, b0[0], a1[0]));
}

// Test more complex expressions with composition
TEST(ImportChp, ComplexComposition) {
	hse::graph g = load_hse_string("(a+; b+) || (c+; d+)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 4);  // a, b, c, d
	EXPECT_GE(g.transitions.size(), 4u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	int d = g.netIndex("d");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	EXPECT_GE(c, 0);
	EXPECT_GE(d, 0);

	vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
	vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
	vector<petri::iterator> c1 = find_transitions(g, boolean::cover(c, 1));
	vector<petri::iterator> d1 = find_transitions(g, boolean::cover(d, 1));
	
	EXPECT_FALSE(a1.empty());
	EXPECT_FALSE(b1.empty());
	EXPECT_FALSE(c1.empty());
	EXPECT_FALSE(d1.empty());
	
	// Verify sequence within each composition
	EXPECT_TRUE(are_sequenced(g, a1[0], b1[0]));
	EXPECT_TRUE(are_sequenced(g, c1[0], d1[0]));
	
	// Verify independence between compositions
	EXPECT_FALSE(are_sequenced(g, a1[0], c1[0]));
	EXPECT_FALSE(are_sequenced(g, c1[0], a1[0]));
	EXPECT_FALSE(are_sequenced(g, b1[0], d1[0]));
	EXPECT_FALSE(are_sequenced(g, d1[0], b1[0]));
}

// Test nested control structures
TEST(ImportChp, NestedControls) {
	hse::graph g = load_hse_string("*[[a -> b+; b- [] ~a -> c+; (d+, e+); c-; (d-, e-)]]");
	
	// Verify the graph structure
	EXPECT_GT(g.netCount(), 4);  // a, b, c, d, e
	EXPECT_GT(g.transitions.size(), 6u);  // At least b+, b-, c+, c-, d+/-, e+/-
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	int d = g.netIndex("d");
	int e = g.netIndex("e");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	EXPECT_GE(c, 0);
	EXPECT_GE(d, 0);
	EXPECT_GE(e, 0);

	vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
	vector<petri::iterator> b0 = find_transitions(g, boolean::cover(b, 0));
	vector<petri::iterator> c1 = find_transitions(g, boolean::cover(c, 1));
	vector<petri::iterator> c0 = find_transitions(g, boolean::cover(c, 0));
	vector<petri::iterator> d1 = find_transitions(g, boolean::cover(d, 1));
	vector<petri::iterator> d0 = find_transitions(g, boolean::cover(d, 0));
	vector<petri::iterator> e1 = find_transitions(g, boolean::cover(e, 1));
	vector<petri::iterator> e0 = find_transitions(g, boolean::cover(e, 0));
	
	EXPECT_FALSE(b1.empty());
	EXPECT_FALSE(b0.empty());
	EXPECT_FALSE(c1.empty());
	EXPECT_FALSE(c0.empty());
	EXPECT_FALSE(d1.empty());
	EXPECT_FALSE(d0.empty());
	EXPECT_FALSE(e1.empty());
	EXPECT_FALSE(e0.empty());
	
	// Verify loops - all transitions should be part of a cycle
	EXPECT_TRUE(are_sequenced(g, b1[0], b1[0]));
	EXPECT_TRUE(are_sequenced(g, c1[0], c1[0]));
	
	// Verify sequence in first branch
	EXPECT_TRUE(are_sequenced(g, b1[0], b0[0]));
	
	// Verify sequence in second branch
	EXPECT_TRUE(are_sequenced(g, c1[0], c0[0]));
	
	// Verify parallel operations
	EXPECT_FALSE(are_sequenced(g, d1[0], e1[0]));
	EXPECT_FALSE(are_sequenced(g, e1[0], d1[0]));
	EXPECT_FALSE(are_sequenced(g, d0[0], e0[0]));
	EXPECT_FALSE(are_sequenced(g, e0[0], d0[0]));
	
	// Verify guards on selection branches
	EXPECT_FALSE(g.transitions[b1[0].index].guard.is_tautology());
	EXPECT_FALSE(g.transitions[c1[0].index].guard.is_tautology());
} 
