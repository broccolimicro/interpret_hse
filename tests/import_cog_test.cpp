#include <common/standard.h>
#include <gtest/gtest.h>

#include <hse/graph.h>
#include <parse/parse.h>
#include <parse/tokenizer.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse_cog/composition.h>
#include <parse_cog/branch.h>
#include <parse_cog/control.h>
#include <parse_cog/factory.h>
#include <interpret_hse/import_cog.h>

#include "helpers.h"

using namespace std;
using namespace test;

// Helper function to parse a string into an HSE graph via COG
hse::graph load_cog_string(string input) {
    // Set up tokenizer
    tokenizer tokens;
    tokens.register_token<parse::block_comment>(false);
    tokens.register_token<parse::line_comment>(false);
    parse_cog::composition::register_syntax(tokens);
    
    tokens.insert("string_input", input, nullptr);
    
    hse::graph g;
    
    // Parse input
    tokens.increment(false);
    tokens.expect<parse_cog::composition>();
    
    if (tokens.decrement(__FILE__, __LINE__)) {
        parse_cog::composition syntax(tokens);
        boolean::cover covered;
        bool hasRepeat = false;
        g = hse::import_hse(syntax, covered, hasRepeat, 0, &tokens, true);
    }
    
    return g;
}

// Test basic sequence import 
TEST(CogImport, BasicSequence) {
    hse::graph g = load_cog_string(R"(
        region 1 {
            a+
            b+
            a-
            b-
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 2);  // a and b
    EXPECT_GE(g.transitions.size(), 4u);  // a+, b+, a-, b-
    
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
    
    // Verify sequence: a+ -> b+ -> a- -> b-
    EXPECT_TRUE(are_sequenced(g, a1[0], b1[0]));
    EXPECT_TRUE(are_sequenced(g, b1[0], a0[0]));
    EXPECT_TRUE(are_sequenced(g, a0[0], b0[0]));
}

// Test parallel composition
TEST(CogImport, ParallelComposition) {
    hse::graph g = load_cog_string(R"(
        {
            a+
            b+
        } and {
            c+
            d+
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 4);  // a, b, c, d
    EXPECT_GE(g.transitions.size(), 4u);  // a+, b+, c+, d+
    
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
    
    // Verify sequencing within each branch
    EXPECT_TRUE(are_sequenced(g, a1[0], b1[0]));
    EXPECT_TRUE(are_sequenced(g, c1[0], d1[0]));
    
    // Verify parallelism between branches
    EXPECT_FALSE(are_sequenced(g, a1[0], c1[0]));
    EXPECT_FALSE(are_sequenced(g, a1[0], d1[0]));
    EXPECT_FALSE(are_sequenced(g, b1[0], c1[0]));
    EXPECT_FALSE(are_sequenced(g, b1[0], d1[0]));
}

// Test conditional waiting
TEST(CogImport, ConditionalWaiting) {
    hse::graph g = load_cog_string(R"(
        {
            await a {
                b+
            }
            await ~a {
                c+
            }
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 3);  // a, b, c
    EXPECT_GE(g.transitions.size(), 2u);  // b+, c+
    
    int a = g.netIndex("a");
    int b = g.netIndex("b");
    int c = g.netIndex("c");
    EXPECT_GE(a, 0);
    EXPECT_GE(b, 0);
    EXPECT_GE(c, 0);
    
    vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
    vector<petri::iterator> c1 = find_transitions(g, boolean::cover(c, 1));
    
    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(c1.empty());
    
    // Verify transitions have guards
    EXPECT_FALSE(g.transitions[b1[0].index].guard.is_tautology());
    EXPECT_FALSE(g.transitions[c1[0].index].guard.is_tautology());
    
    // b+ and c+ should not be sequenced (they're from different branches)
    EXPECT_FALSE(are_sequenced(g, b1[0], c1[0]));
    EXPECT_FALSE(are_sequenced(g, c1[0], b1[0]));
}

// Test while loop
TEST(CogImport, WhileLoop) {
    hse::graph g = load_cog_string(R"(
        region 1 {
            while {
                a+
                b+
                a-
                b-
            }
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 2);  // a and b
    EXPECT_GE(g.transitions.size(), 4u);  // a+, b+, a-, b-
    
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
    EXPECT_TRUE(are_sequenced(g, b0[0], a1[0]));
    EXPECT_TRUE(are_sequenced(g, a1[0], b1[0]));
    EXPECT_TRUE(are_sequenced(g, b1[0], a0[0]));
    EXPECT_TRUE(are_sequenced(g, a0[0], b0[0]));
}

// Test COG-specific 'await' construct
TEST(CogImport, AwaitConstruct) {
    hse::graph g = load_cog_string(R"(
        region 1 {
            a+
            await b
            a-
            await ~b
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 2);  // a and b
    EXPECT_GE(g.transitions.size(), 2u);  // a+, a-
    
    int a = g.netIndex("a");
    int b = g.netIndex("b");
    EXPECT_GE(a, 0);
    EXPECT_GE(b, 0);
    
    vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
    vector<petri::iterator> a0 = find_transitions(g, boolean::cover(a, 0));
    
    EXPECT_FALSE(a1.empty());
    EXPECT_FALSE(a0.empty());
    
    // a+ and a- should be sequenced
    EXPECT_TRUE(are_sequenced(g, a1[0], a0[0]));
    
    // Verify transitions have appropriate guards
    EXPECT_FALSE(g.transitions[a0[0].index].guard.is_tautology());
}

// Test WCHB buffer from example
TEST(CogImport, WCHB1bBuffer) {
    hse::graph g = load_cog_string(R"(
        region 1 {
            L.f- and L.t-
            await L.e
            while {
                L.f+ xor L.t+
                await ~L.e
                L.f- and L.t-
                await L.e
            }
        } and {
            L.e+ and R.f- and R.t-
            await R.e & ~L.f & ~L.t
            while {
                await R.e & L.f {
                    R.f+
                } or await R.e & L.t {
                    R.t+
                }
                L.e-
                await ~R.e & ~L.f & ~L.t
                R.f- and R.t-
                L.e+
            }
        } and region 1 {
            R.e+
            await ~R.f & ~R.t
            while {
                await R.f | R.t
                R.e-
                await ~R.f & ~R.t
                R.e+
            }
        }
    )");
    
    // Verify the graph structure
    EXPECT_GT(g.netCount(), 3);  // L.e, L.f, L.t, R.e, R.f, R.t
    EXPECT_GT(g.transitions.size(), 6u);  // Various transitions for each signal
    
    // Check for key signals
    int le = g.netIndex("L.e");
    int lf = g.netIndex("L.f");
    int lt = g.netIndex("L.t");
    int re = g.netIndex("R.e");
    int rf = g.netIndex("R.f");
    int rt = g.netIndex("R.t");
    
    EXPECT_GE(le, 0);
    EXPECT_GE(lf, 0);
    EXPECT_GE(lt, 0);
    EXPECT_GE(re, 0);
    EXPECT_GE(rf, 0);
    EXPECT_GE(rt, 0);
    
    // Check for transitions for all signals
    vector<petri::iterator> le1 = find_transitions(g, boolean::cover(le, 1));
    vector<petri::iterator> le0 = find_transitions(g, boolean::cover(le, 0));
    vector<petri::iterator> lf1 = find_transitions(g, boolean::cover(lf, 1));
    vector<petri::iterator> lf0 = find_transitions(g, boolean::cover(lf, 0));
    vector<petri::iterator> lt1 = find_transitions(g, boolean::cover(lt, 1));
    vector<petri::iterator> lt0 = find_transitions(g, boolean::cover(lt, 0));
    vector<petri::iterator> re1 = find_transitions(g, boolean::cover(re, 1));
    vector<petri::iterator> re0 = find_transitions(g, boolean::cover(re, 0));
    vector<petri::iterator> rf1 = find_transitions(g, boolean::cover(rf, 1));
    vector<petri::iterator> rf0 = find_transitions(g, boolean::cover(rf, 0));
    vector<petri::iterator> rt1 = find_transitions(g, boolean::cover(rt, 1));
    vector<petri::iterator> rt0 = find_transitions(g, boolean::cover(rt, 0));
    
    EXPECT_FALSE(le1.empty());
    EXPECT_FALSE(le0.empty());
    EXPECT_FALSE(lf1.empty());
    EXPECT_FALSE(lf0.empty());
    EXPECT_FALSE(lt1.empty());
    EXPECT_FALSE(lt0.empty());
    EXPECT_FALSE(re1.empty());
    EXPECT_FALSE(re0.empty());
    EXPECT_FALSE(rf1.empty());
    EXPECT_FALSE(rf0.empty());
    EXPECT_FALSE(rt1.empty());
    EXPECT_FALSE(rt0.empty());
    
    // Verify key properties of the WCHB buffer:
    
    // 1. Left handshake: L.f+ (or L.t+) should be followed by L.e-
    bool left_handshake = are_sequenced(g, lf1[0], le0[0]) || are_sequenced(g, lt1[0], le0[0]);
    EXPECT_TRUE(left_handshake);
    
    // 2. Right handshake: R.f+ (or R.t+) should be followed by R.e-
    bool right_handshake = are_sequenced(g, rf1[0], re0[0]) || are_sequenced(g, rt1[0], re0[0]);
    EXPECT_TRUE(right_handshake);
    
    // 3. Check cycles - signals should form a loop
    EXPECT_TRUE(are_sequenced(g, le1[0], le1[0]));
    EXPECT_TRUE(are_sequenced(g, re1[0], re1[0]));
}

// Test alternative 'or' construct in COG
TEST(CogImport, OrConstruct) {
    hse::graph g = load_cog_string(R"(
        region 1 {
            await a {
                b+
            } or await c {
                d+
            }
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 4);  // a, b, c, d
    EXPECT_GE(g.transitions.size(), 2u);  // b+, d+
    
    int a = g.netIndex("a");
    int b = g.netIndex("b");
    int c = g.netIndex("c");
    int d = g.netIndex("d");
    
    EXPECT_GE(a, 0);
    EXPECT_GE(b, 0);
    EXPECT_GE(c, 0);
    EXPECT_GE(d, 0);
    
    vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
    vector<petri::iterator> d1 = find_transitions(g, boolean::cover(d, 1));
    
    EXPECT_FALSE(b1.empty());
    EXPECT_FALSE(d1.empty());
    
    // Verify transitions have guards
    EXPECT_FALSE(g.transitions[b1[0].index].guard.is_tautology());
    EXPECT_FALSE(g.transitions[d1[0].index].guard.is_tautology());
    
    // b+ and d+ should not be sequenced (they're mutual exclusive)
    EXPECT_FALSE(are_sequenced(g, b1[0], d1[0]));
    EXPECT_FALSE(are_sequenced(g, d1[0], b1[0]));
}

// Test 'xor' expression in COG
TEST(CogImport, XorExpression) {
    hse::graph g = load_cog_string(R"(
        region 1 {
            a+ xor b+
        }
    )");
    
    // Verify the graph structure
    EXPECT_EQ(g.netCount(), 2);  // a and b
    EXPECT_GE(g.transitions.size(), 2u);  // a+, b+
    
    int a = g.netIndex("a");
    int b = g.netIndex("b");
    
    EXPECT_GE(a, 0);
    EXPECT_GE(b, 0);
    
    vector<petri::iterator> a1 = find_transitions(g, boolean::cover(a, 1));
    vector<petri::iterator> b1 = find_transitions(g, boolean::cover(b, 1));
    
    EXPECT_FALSE(a1.empty());
    EXPECT_FALSE(b1.empty());
    
    // a+ and b+ should be mutual exclusive
    EXPECT_FALSE(are_sequenced(g, a1[0], b1[0]));
    EXPECT_FALSE(are_sequenced(g, b1[0], a1[0]));
} 
