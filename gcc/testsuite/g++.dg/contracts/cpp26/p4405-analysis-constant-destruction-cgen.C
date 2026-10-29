// D4405R0 ("The Analysis Evaluation"; DECISIONS.md E142): there is no
// analysis evaluation during constant evaluation.  A constexpr or
// constinit variable's initialization is manifestly constant-evaluated,
// and so is the destructor call of a constexpr variable's constant
// destruction: none of those calls has an analysis evaluation, so a
// precondition that holds in constant evaluation
// (__builtin_is_constant_evaluated ()) and that folding cannot prove
// outside it is not diagnosed.  The constant evaluation itself evaluates
// the prove precondition as enforce, and it holds.  GCC-943: the cleanups
// of the temporaries cp_finish_decl's value initialization builds, and the
// constant destruction's destructor call, were diagnosed as unproven --
// four times for `ce1', at code generation only, so this test is not
// -fsyntax-only (p4405-analysis-address-static-dtor.C is).
//
// Still diagnosed: an object destroyed at run time -- a non-constexpr
// object of static storage duration (constinit too) through its
// destructor's address (E146, E149 (a)), and an automatic one where its
// destructor is called, once each.  So is the call of `~DE' in the body of
// H's implicit destructor, which constant destruction of `ch' defines: a
// call in a function body, diagnosed once, at the class, like the same
// call in any constexpr function.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
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

int g;

struct DE {
  constexpr ~DE () pre<always_prove> (__builtin_is_constant_evaluated () || g > 0) {}	// { dg-message "precondition here" }
};
struct H {	// { dg-error "contract assertion cannot be proven" }
  DE d;
};

constexpr DE ce1;
constexpr DE ce2{};
constexpr DE ce3 = DE ();
constexpr DE ca[2];
constexpr H ch;
constexpr const DE &cr = ce1;

constinit DE ci;	// { dg-error "contract assertion cannot be proven for a call through the address of 'constexpr DE::~DE\\(\\)'" }
// { dg-message "'ci', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
DE e1;	// { dg-error "contract assertion cannot be proven for a call through the address of 'constexpr DE::~DE\\(\\)'" }
// { dg-message "'e1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

struct S {
  static constexpr DE sm{};
};

template <class T> constexpr T vt{};
template <class T>
void
tf ()
{
  constexpr T a;
  static constexpr T s;
}

void
f ()
{
  constexpr DE a;
  constexpr DE b{};
  constexpr DE c[2];
  static constexpr DE s;
  (void) &vt<DE>;
  (void) &S::sm;
  tf<DE> ();
}

void
h ()
{
  DE d;	// { dg-error "contract assertion cannot be proven" }
}
