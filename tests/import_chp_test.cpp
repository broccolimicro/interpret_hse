#include <common/standard.h>
#include <gtest/gtest.h>

#include <hse/graph.h>
#include <parse/tokenizer.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse_chp/composition.h>
#include <interpret_hse/import_chp.h>
#include <interpret_hse/export_dot.h>

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
		hse::import_hse(g, syntax, &tokens, true);
	}

	return g;
}

// Test basic sequence import (a+; b+; a-; b-)
TEST(ChpImport, Sequence) {
	hse::graph g = load_hse_string("a+; b+; c-; d-");

	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 4);
	EXPECT_EQ(g.transitions.size(), 4u);
	EXPECT_EQ(g.places.size(), 4u);

	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	int d = g.netIndex("d");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);

	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(c, 0));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(d, 0));
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);

	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
}

// Test parallel composition import ((a+, b+); (a-, b-))
TEST(ChpImport, Parallel) {
	hse::graph g = load_hse_string("(a+, b+); (a-, b-)");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 5u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);

	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(a, 0));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(b, 0));
	vector<petri::iterator> sp = findRule(g, 1, 1);
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	ASSERT_EQ(sp.size(), 1u);

	// Verify parallel structure - a+ and b+ should be concurrent
	EXPECT_TRUE(g.is_parallel(a1[0], b1[0]));
	EXPECT_TRUE(g.is_parallel(a0[0], b0[0]));
	
	// Verify sequencing - first parallel group completes before second group
	EXPECT_TRUE(g.is_sequence(a1[0], sp[0]));
	EXPECT_TRUE(g.is_sequence(sp[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], sp[0]));
	EXPECT_TRUE(g.is_sequence(sp[0], b0[0]));
}

// Test selection import ([c -> a+; a- [] ~c -> b+; b-])
TEST(ChpImport, Selection) {
	hse::graph g = load_hse_string("[c -> a+; a- [] ~c -> b+; b-]");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 3);  // a, b, and c
	EXPECT_EQ(g.transitions.size(), 6u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	int c = g.netIndex("c");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);
	EXPECT_GE(c, 0);

	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(a, 0));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(b, 0));
	vector<petri::iterator> c1 = findRule(g, boolean::cover(c, 1), 1);
	vector<petri::iterator> c0 = findRule(g, boolean::cover(c, 0), 1);

	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	ASSERT_EQ(c1.size(), 1u);
	ASSERT_EQ(c0.size(), 1u);
	
	// Verify selection structure (no path between a+ and b+)
	EXPECT_TRUE(g.is_choice(c1[0], c0[0]));
	EXPECT_TRUE(g.is_choice(a1[0], b1[0]));
	EXPECT_TRUE(g.is_choice(a0[0], b0[0]));
	
	// Verify each branch internal sequencing
	EXPECT_TRUE(g.is_sequence(c1[0], a1[0]));
	EXPECT_TRUE(g.is_sequence(a1[0], a0[0]));

	EXPECT_TRUE(g.is_sequence(c0[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], b0[0]));
}

// Test loop import (*[a+; b+; a-; b-])
TEST(ChpImport, Loop) {
	hse::graph g = load_hse_string("*[a+; b+; a-; b-]");
	
	// Verify the graph structure
	EXPECT_EQ(g.netCount(), 2);
	EXPECT_EQ(g.transitions.size(), 4u);
	
	int a = g.netIndex("a");
	int b = g.netIndex("b");
	EXPECT_GE(a, 0);
	EXPECT_GE(b, 0);

	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> a0 = findRule(g, 1, boolean::cover(a, 0));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(b, 0));
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	
	// Verify cycle: should be able to go from any transition back to itself
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(b1[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], b0[0]));
	EXPECT_TRUE(g.is_sequence(b0[0], a1[0]));
}

// Test more complex expressions with composition
TEST(ChpImport, ComplexComposition) {
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

	vector<petri::iterator> a1 = findRule(g, 1, boolean::cover(a, 1));
	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> c1 = findRule(g, 1, boolean::cover(c, 1));
	vector<petri::iterator> d1 = findRule(g, 1, boolean::cover(d, 1));
	
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(c1.size(), 1u);
	ASSERT_EQ(d1.size(), 1u);
	
	// Verify sequence within each composition
	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], d1[0]));
	
	// Verify independence between compositions
	EXPECT_TRUE(g.is_parallel(a1[0], c1[0]));
	EXPECT_TRUE(g.is_parallel(c1[0], a1[0]));
	EXPECT_TRUE(g.is_parallel(b1[0], d1[0]));
	EXPECT_TRUE(g.is_parallel(d1[0], b1[0]));
}

// Test nested control structures
TEST(ChpImport, NestedControls) {
	hse::graph g = load_hse_string("*[[a -> b+; b- [] ~a -> c+; (d+, e+); c-; (d-, e-)]]");
	
	// Verify the graph structure
	EXPECT_GT(g.netCount(), 4);  // a, b, c, d, e
	EXPECT_GT(g.transitions.size(), 10u);  // At least b+, b-, c+, c-, d+/-, e+/-
	
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

	vector<petri::iterator> b1 = findRule(g, 1, boolean::cover(b, 1));
	vector<petri::iterator> b0 = findRule(g, 1, boolean::cover(b, 0));
	vector<petri::iterator> c1 = findRule(g, 1, boolean::cover(c, 1));
	vector<petri::iterator> c0 = findRule(g, 1, boolean::cover(c, 0));
	vector<petri::iterator> d1 = findRule(g, 1, boolean::cover(d, 1));
	vector<petri::iterator> d0 = findRule(g, 1, boolean::cover(d, 0));
	vector<petri::iterator> e1 = findRule(g, 1, boolean::cover(e, 1));
	vector<petri::iterator> e0 = findRule(g, 1, boolean::cover(e, 0));
	vector<petri::iterator> a0 = findRule(g, boolean::cover(a, 0), 1);
	vector<petri::iterator> a1 = findRule(g, boolean::cover(a, 1), 1);
	vector<petri::iterator> sp = findRule(g, 1, 1);
	
	ASSERT_EQ(b1.size(), 1u);
	ASSERT_EQ(b0.size(), 1u);
	ASSERT_EQ(c1.size(), 1u);
	ASSERT_EQ(c0.size(), 1u);
	ASSERT_EQ(d1.size(), 1u);
	ASSERT_EQ(d0.size(), 1u);
	ASSERT_EQ(e1.size(), 1u);
	ASSERT_EQ(e0.size(), 1u);
	ASSERT_EQ(a0.size(), 1u);
	ASSERT_EQ(a1.size(), 1u);
	ASSERT_EQ(sp.size(), 1u);
	
	// Verify loops - all transitions should be part of a cycle
	EXPECT_TRUE(g.is_sequence(b1[0], b0[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], d1[0]));
	EXPECT_TRUE(g.is_sequence(c1[0], e1[0]));
	EXPECT_TRUE(g.is_sequence(d1[0], c0[0]));
	EXPECT_TRUE(g.is_sequence(e1[0], c0[0]));
	EXPECT_TRUE(g.is_sequence(c0[0], d0[0]));
	EXPECT_TRUE(g.is_sequence(c0[0], e0[0]));

	EXPECT_TRUE(g.is_sequence(d0[0], sp[0]));
	EXPECT_TRUE(g.is_sequence(e0[0], sp[0]));

	EXPECT_TRUE(g.is_sequence(sp[0], a0[0]));
	EXPECT_TRUE(g.is_sequence(b0[0], a1[0]));

	EXPECT_TRUE(g.is_sequence(a1[0], b1[0]));
	EXPECT_TRUE(g.is_sequence(a0[0], c1[0]));
} 
