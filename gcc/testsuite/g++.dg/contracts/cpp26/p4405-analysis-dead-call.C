// P4405, DECISIONS.md E169 (13), (14): a call in ordinary code that folding
// proves is never evaluated has no analysis evaluation at the call under
// -fcontract-analysis-skip-dead-calls, which -fcontracts-p4405 implies: an
// operand of `?:' that the condition never selects, the second operand of
// `&&' or `||' behind a constant, a substatement of `if' that a constant
// condition never selects, and a discarded statement.  The condition is
// decided by full folding, so `1 ?' and `0 &&' count as `true ?' and
// `false &&' do, and so does a `constexpr' variable; a template decides
// where it is instantiated, as a non-template does (`t*').  The operand
// that is evaluated is analyzed (`live*').  So is any call behind a
// condition that is not a constant expression (`h4', `th4'), which the
// front end does not skip; but at this -O2 the proof shadow finds it
// never reached, and a call never reached is proven (DECISIONS.md E195;
// at -O0: p4405-unreachable-O0.C).  So is a statement after a return
// (`a1'), up to a label (DECISIONS.md E195, GCC-992).  The nested walk of a predicate
// skips such an operand whatever the option says (`h7', `h8': no note
// for g3's precondition).  -fno-contract-analysis-skip-dead-calls:
// p4405-analysis-dead-call-no-skip.C.
//
// Differences from Clang, which skips what its code generation leaves out
// (E169 (18): each compiler's own permission; pinned, not aligned):
// - `s1': a case of a constant switch never reached is not a folded-away
//   operand here, so the front end analyzes it (its label makes it
//   reachable as far as the front end knows); the proof shadow then finds
//   it never reached (at -O0 it is always false).
// - `lam': a lambda's body is a function of its own here, analyzed as any
//   other, even in a dead arm; Clang never emits it, so never analyzes it.
// - `agg': the dead arm of a `?:' of class type is skipped here as any
//   other dead arm; Clang emits it all the same and analyzes it.
//
// Mirror: clang/test/Contracts/p4405-analysis-dead-call.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }

int g3 (int x) pre (x != 0);	// { dg-message "the predicate of the contract assertion" }
				// { dg-bogus "precondition of a call" "" { target *-*-* } .-1 }
int g3 (int x) { return 10 / x; }
int g4 (int x) pre (x != 0);
int g4 (int x) { return 10 / x; }

constexpr bool k = false;

// Never evaluated: not analyzed.
int h1 () { return 1 ? g4 (1) : g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
int h1b () { return 0 ? g3 (0) : 2; }		// { dg-bogus "(cannot be proven|always false)" }
bool h2 () { return false && g3 (0); }		// { dg-bogus "(cannot be proven|always false)" }
bool h2b () { return true || g3 (0); }		// { dg-bogus "(cannot be proven|always false)" }
bool h2c () { return 0 && g3 (0); }		// { dg-bogus "(cannot be proven|always false)" }
bool h2d () { return 1 || g3 (0); }		// { dg-bogus "(cannot be proven|always false)" }
int h3 (int y) { if (0) return g3 (0); return y; }	// { dg-bogus "(cannot be proven|always false)" }
int h3b (int y) { if (1) return y; else return g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
int h5 () { return k ? g3 (0) : 2; }		// { dg-bogus "(cannot be proven|always false)" }
int h5b (int y) { if (k) return g3 (0); return y; }	// { dg-bogus "(cannot be proven|always false)" }
int h6 () { if constexpr (false) return g3 (0); return 1; }	// { dg-bogus "(cannot be proven|always false)" }
int h6b () { if constexpr (true) return 1; else return g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
bool h9 () { return 1 || 0 && g3 (0); }		// { dg-bogus "(cannot be proven|always false)" }
bool h9b () { return true || (false && g3 (0)); }	// { dg-bogus "(cannot be proven|always false)" }
int h9c (int y) { return y ? (0 ? g3 (0) : 1) : 2; }	// { dg-bogus "(cannot be proven|always false)" }
struct S { int v; S (int v) : v (v) {} };
S mk (int);
S agg () { return 1 ? S (1) : mk (g3 (0)); }	// { dg-bogus "(cannot be proven|always false)" }

// Never evaluated: skipped after a return; proven never reached by the
// shadow in a case of a constant switch.
int a1 (int y) { return y; g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
int s1 () { switch (1) { case 0: return g3 (0); case 1: return 1; } return 0; }	// { dg-bogus "(cannot be proven|always false)" }
// Never evaluated, but analyzed all the same (Clang skips these).
int lam () { return 0 ? [] { return g3 (0); } () : 1; }	// { dg-error "contract predicate is always false" }

// Evaluated, or not known never to be: analyzed.
int live1 () { return 0 ? 2 : g3 (0); }		// { dg-error "contract predicate is always false" }
bool live2 () { return true && g3 (0); }	// { dg-error "contract predicate is always false" }
bool live3 () { return 0 || g3 (0); }		// { dg-error "contract predicate is always false" }
int live4 (int y) { if (0) return y; else return g3 (0); }	// { dg-error "contract predicate is always false" }
int live5 (int y) { if (1) return g3 (0); return y; }	// { dg-error "contract predicate is always false" }
int live6 (int y) { if (0) return y; return g3 (0); }	// { dg-error "contract predicate is always false" }
bool live7 () { return 0 && g3 (0) || g3 (0); }	// { dg-error "contract predicate is always false" }
int h4 (int y) { return y > 0 && y < 0 ? g3 (0) : y; }	// { dg-bogus "(cannot be proven|always false)" }

// Predicates: the nested walk, unchanged.
int h7 (int y) { contract_assert ((1 ? g4 (1) : g3 (0)) == 10); return y; }	// { dg-bogus "(cannot be proven|always false)" }
int h8 (int y) { contract_assert (false && g3 (0)); return y; }	// { dg-error "contract predicate is always false" }

// Templates, decided where they are instantiated: a non-dependent
// condition, and a dependent one.
template <typename T> int t1 () { return 1 ? g4 (1) : g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int t1b () { return 0 ? g3 (0) : 2; }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> bool t2 () { return false && g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> bool t2b () { return true || g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> bool t2c () { return 0 && g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int t3 (int y) { if (0) return g3 (0); return y; }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int t3b (int y) { if (1) return y; else return g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int t5 () { return k ? g3 (0) : 2; }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int t6 () { if constexpr (false) return g3 (0); return 1; }	// { dg-bogus "(cannot be proven|always false)" }
template <int N> int tn1 () { return N ? g4 (1) : g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <int N> bool tn2 () { return N && g3 (0); }	// { dg-bogus "(cannot be proven|always false)" }
template <int N> int tn3 (int y) { if (N) return g3 (0); return y; }	// { dg-bogus "(cannot be proven|always false)" }
template <int N> int tn6 () { if constexpr (N) return g3 (0); return 1; }	// { dg-bogus "(cannot be proven|always false)" }
template <typename T> int tlive1 () { return 0 ? 2 : g3 (0); }	// { dg-error "contract predicate is always false" }
template <int N> bool tlive2 () { return N && g3 (0); }	// { dg-error "contract predicate is always false" }
template <typename T> int th4 (int y) { return y > 0 && y < 0 ? g3 (0) : y; }	// { dg-bogus "(cannot be proven|always false)" }

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
