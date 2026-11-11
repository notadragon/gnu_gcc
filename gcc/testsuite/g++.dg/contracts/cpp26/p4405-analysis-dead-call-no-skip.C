// P4405, DECISIONS.md E169 (13), (14): -fno-contract-analysis-skip-dead-calls
// gives a call that folding proves is never evaluated its analysis
// evaluation at the call, as any other call: every dead call of
// p4405-analysis-dead-call.C is analyzed here, and g3's precondition is
// always false there, in templates and non-templates alike (but for the
// discarded statement of a template, which is never instantiated).  The
// nested walk of a predicate skips such an operand whatever the option
// says (`h7', `h8').
//
// At proof level 0, so that only the front end decides: a proof shadow
// finds most of these calls never reached, and a call never
// reached is proven (DECISIONS.md E195), whatever this option says.  So
// `h7''s assertion, whose predicate calls g4, cannot be proven here (the
// shadow proves it); the nested walk still skips g3 (0), with no note.
//
// Differences from Clang (E169 (18); pinned, not aligned): `lam', a
// lambda's body in a dead arm, is a function of its own here and analyzed
// as any other; Clang never emits it, so never analyzes it even so.  `a1',
// `s1' and `agg' are analyzed here as under Clang (`a1' and `s1' whatever
// the option says: p4405-analysis-dead-call.C).
//
// Mirror: clang/test/Contracts/p4405-analysis-dead-call-no-skip.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-additional-options "-fno-contract-analysis-skip-dead-calls" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int g3 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
				// { dg-bogus "precondition of a call" "" { target *-*-* } .-1 }
int g3 (int x) { return 10 / x; }
int g4 (int x) pre (x != 0);
int g4 (int x) { return 10 / x; }

constexpr bool k = false;

// Never evaluated: analyzed all the same.
int h1 () { return 1 ? g4 (1) : g3 (0); }	// { dg-error "contract predicate is always false" }
int h1b () { return 0 ? g3 (0) : 2; }		// { dg-error "contract predicate is always false" }
bool h2 () { return false && g3 (0); }		// { dg-error "contract predicate is always false" }
bool h2b () { return true || g3 (0); }		// { dg-error "contract predicate is always false" }
bool h2c () { return 0 && g3 (0); }		// { dg-error "contract predicate is always false" }
bool h2d () { return 1 || g3 (0); }		// { dg-error "contract predicate is always false" }
int h3 (int y) { if (0) return g3 (0); return y; }	// { dg-error "contract predicate is always false" }
int h3b (int y) { if (1) return y; else return g3 (0); }	// { dg-error "contract predicate is always false" }
int h5 () { return k ? g3 (0) : 2; }		// { dg-error "contract predicate is always false" }
int h5b (int y) { if (k) return g3 (0); return y; }	// { dg-error "contract predicate is always false" }
int h6 () { if constexpr (false) return g3 (0); return 1; }	// { dg-error "contract predicate is always false" }
int h6b () { if constexpr (true) return 1; else return g3 (0); }	// { dg-error "contract predicate is always false" }
bool h9 () { return 1 || 0 && g3 (0); }		// { dg-error "contract predicate is always false" }
bool h9b () { return true || (false && g3 (0)); }	// { dg-error "contract predicate is always false" }
int h9c (int y) { return y ? (0 ? g3 (0) : 1) : 2; }	// { dg-error "contract predicate is always false" }
int a1 (int y) { return y; g3 (0); }	// { dg-error "contract predicate is always false" }
int s1 () { switch (1) { case 0: return g3 (0); case 1: return 1; } return 0; }	// { dg-error "contract predicate is always false" }
int lam () { return 0 ? [] { return g3 (0); } () : 1; }	// { dg-error "contract predicate is always false" }
struct S { int v; S (int v) : v (v) {} };
S mk (int);
S agg () { return 1 ? S (1) : mk (g3 (0)); }	// { dg-error "contract predicate is always false" }

// Evaluated, or not known never to be: analyzed.
int live1 () { return 0 ? 2 : g3 (0); }		// { dg-error "contract predicate is always false" }
bool live2 () { return true && g3 (0); }	// { dg-error "contract predicate is always false" }
bool live3 () { return 0 || g3 (0); }		// { dg-error "contract predicate is always false" }
int live4 (int y) { if (0) return y; else return g3 (0); }	// { dg-error "contract predicate is always false" }
int live5 (int y) { if (1) return g3 (0); return y; }	// { dg-error "contract predicate is always false" }
int live6 (int y) { if (0) return y; return g3 (0); }	// { dg-error "contract predicate is always false" }
bool live7 () { return 0 && g3 (0) || g3 (0); }	// { dg-error "contract predicate is always false" }
int h4 (int y) { return y > 0 && y < 0 ? g3 (0) : y; }	// { dg-error "contract predicate is always false" }

// Predicates: the nested walk, unchanged.
int h7 (int y) { contract_assert ((1 ? g4 (1) : g3 (0)) == 10); return y; }	// { dg-error "contract assertion cannot be proven" }
int h8 (int y) { contract_assert (false && g3 (0)); return y; }	// { dg-error "contract predicate is always false" }

// Templates: a non-dependent condition, and a dependent one.  The
// discarded statement of a template's `if constexpr' is never instantiated
// (`t6', `tn6'), so it has no calls to analyze.
template <typename T> int t1 () { return 1 ? g4 (1) : g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> int t1b () { return 0 ? g3 (0) : 2; }	// { dg-error "contract predicate is always false" }
template <typename T> bool t2 () { return false && g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> bool t2b () { return true || g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> bool t2c () { return 0 && g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> int t3 (int y) { if (0) return g3 (0); return y; }	// { dg-error "contract predicate is always false" }
template <typename T> int t3b (int y) { if (1) return y; else return g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> int t5 () { return k ? g3 (0) : 2; }	// { dg-error "contract predicate is always false" }
template <typename T> int t6 () { if constexpr (false) return g3 (0); return 1; }	// { dg-bogus "(cannot be proven|always false)" }
template <int N> int tn1 () { return N ? g4 (1) : g3 (0); }	// { dg-error "contract predicate is always false" }
template <int N> bool tn2 () { return N && g3 (0); }	// { dg-error "contract predicate is always false" }
template <int N> int tn3 (int y) { if (N) return g3 (0); return y; }	// { dg-error "contract predicate is always false" }
template <int N> int tn6 () { if constexpr (N) return g3 (0); return 1; }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int tlive1 () { return 0 ? 2 : g3 (0); }	// { dg-error "contract predicate is always false" }
template <int N> bool tlive2 () { return N && g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> int th4 (int y) { return y > 0 && y < 0 ? g3 (0) : y; }	// { dg-error "contract predicate is always false" }

template int t1<int> ();
template int t1b<int> ();
template bool t2<int> ();
template bool t2b<int> ();
template bool t2c<int> ();
template int t3<int> (int);
template int t3b<int> (int);
template int t5<int> ();
template int t6<int> ();
template int tn1<1> ();
template bool tn2<0> ();
template int tn3<0> (int);
template int tn6<0> ();
template int tlive1<int> ();
template bool tlive2<1> ();
template int th4<int> (int);
