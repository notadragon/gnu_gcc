// D4405R0 ("Taking the Address of a Function"; DECISIONS.md E146 (a)): the
// definition of an object of static or thread storage duration is treated
// as taking the address of its destructor, which the implementation calls
// at exit or when the thread ends, with no caller in the program to analyze
// the call.  So a precondition of the destructor that allows no runtime
// semantic, and that folding does not prove, cannot be proven for that
// call: an error under prove (always_prove) and a -Wcontract-unproven
// warning under diagnose (always_diagnose), at namespace scope, a static
// local, a static data member's definition (out of line, or inline),
// thread_local objects, and arrays of such a class.  Not diagnosed: a
// declaration that is not a definition, an automatic object (its
// destructor call is analyzed where it is made), a destructor whose
// precondition is not analyzed, or is tautological (`DC', `pre (true)'), a
// constexpr object with constant destruction, and a template's definition
// -- its instantiations are, each once.  (GCC-930: no such definition was
// diagnosed.)  GCC has no attribute for an object that is never destroyed
// (Clang's [[clang::no_destroy]]).  -fsyntax-only, as Clang's mirror: the
// analysis evaluations of the calls that are made are not this test's.
//
// Constant destruction (DECISIONS.md E149 (a), the Divergences row
// GCC-934): GCC evaluates constant destruction only for a constexpr
// variable, so it registers the destructor of the non-constexpr `DE ce2;'
// with __cxa_atexit and diagnoses it (its precondition holds in constant
// evaluation, is_constant_evaluated (), but folding outside it does not
// prove it); Clang does not destroy such an object, whose destruction is a
// constant expression, and does not diagnose it.  Each diagnoses the
// objects it actually destroys through the destructor's address.  `DM m2;'
// (a precondition reading a member, which folding cannot prove) is
// diagnosed by both: Clang's constant destruction of a non-constexpr
// object cannot read its members.  `DG g1;', whose destructor precondition
// is not a core constant expression, is destroyed and diagnosed by both.
// Temporaries of static storage duration are
// p4405-analysis-address-static-temp.C; the code generation of arrays,
// p4405-analysis-static-array-dtor.C.
//
// Mirror: clang/test/Contracts/p4405-analysis-address-static-dtor.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fsyntax-only" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int g;

struct DP {
  ~DP () pre<always_prove> (g > 0);	// { dg-message "precondition here" }
};
struct DD {
  ~DD () pre<always_diagnose> (g > 0);	// { dg-message "precondition here" }
};
struct DI {
  ~DI () pre (g > 0);
};
struct DC {
  constexpr ~DC () pre<always_prove> (true) {}	// { dg-bogus "precondition here" }
};
struct DM {
  int v = 1;
  constexpr ~DM () pre<always_prove> (v > 0) {}	// { dg-message "precondition here" }
};
struct DE {
  constexpr ~DE () pre<always_prove> (__builtin_is_constant_evaluated () || g > 0) {}	// { dg-message "precondition here" }
};
struct DG {
  constexpr ~DG () pre<always_prove> (g > 0) {}	// { dg-message "precondition here" }
};

DP p1;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'p1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
DD d1;	// { dg-warning "contract assertion cannot be proven for a call through the address of 'DD::~DD\\(\\)'" }
// { dg-message "'d1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
DI i1;
DP pa[2];	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'pa', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
thread_local DP t1;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'t1', of thread storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
thread_local DD t2;	// { dg-warning "contract assertion cannot be proven for a call through the address of 'DD::~DD\\(\\)'" }
// { dg-message "'t2', of thread storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
extern DP e1;
constexpr DC c1;
DC c2;	// { dg-bogus "address" }
constexpr DM m1;
DM m2;	// { dg-error "contract assertion cannot be proven for a call through the address of 'constexpr DM::~DM\\(\\)'" }
// { dg-message "'m2', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
constexpr DE ce1;
DE ce2;	// { dg-error "contract assertion cannot be proven for a call through the address of 'constexpr DE::~DE\\(\\)'" }
// { dg-message "'ce2', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
DG g1;	// { dg-error "contract assertion cannot be proven for a call through the address of 'constexpr DG::~DG\\(\\)'" }
// { dg-message "'g1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

struct S {
  static DP s1;
  static inline DP s2;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'S::s2', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
};
DP S::s1;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'S::s1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

void
f ()
{
  static DP sl;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'sl', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
  thread_local DP tl;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'tl', of thread storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
  DP a;
  DI ai;
}

template <class T>
void
tf ()
{
  static DP st;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'st', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
}
template <class T> void never () { static DP sn; }

template <class T> struct TS {
  static inline DP si;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'TS<int>::si', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
};

template <class T> DP vt;	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "'vt<int>', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

void
use ()
{
  tf<int> ();
  tf<int> ();
  (void) &TS<int>::si;
  (void) &vt<int>;
}
