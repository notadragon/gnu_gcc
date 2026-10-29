// D4405 ("The Analysis Evaluation"): a constructor or destructor
// precondition that names `this', analyzed at a call, is given the object
// the call constructs or destroys -- for a named object and for a prvalue's
// temporary alike: a prvalue (`T (y)') has no object yet at the call, only
// a placeholder, so the temporary it initializes is made there and its
// slot substituted (GCC-926), which an initialization that elides the
// temporary replaces by the object it initializes -- so a guarded call is
// proven, and an unguarded one is not.  An object's member read by a
// destructor's precondition is the object's.  (GCC-922: the placeholder
// was substituted, an internal compiler error.)
//
// Mirror: clang/test/Contracts/p4405-analysis-ctor-this.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
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

struct T {
  T (int y) pre<always_prove> (this != nullptr && y > 0);	// { dg-message "the predicate of the contract assertion" }
  int v = 0;
};

struct U {
  int v;
  ~U () pre<always_prove> (v > 0);	// { dg-message "the predicate of the contract assertion" }
};

int
object (int y)
{
  if (y > 0)
    {
      T t (y);					// { dg-bogus "cannot be proven" }
      return t.v;
    }
  return 0;
}

int
prvalue (int y)
{
  if (y > 0)
    return T (y).v;				// { dg-bogus "cannot be proven" }
  return 0;
}

int
unguarded (int y)
{
  return T (y).v;				// { dg-error "contract assertion cannot be proven" }
}

void
destroyed (int y)
{
  if (y > 0)
    {
      U u{y};					// { dg-bogus "cannot be proven" }
    }
}

void
destroyed_unguarded (int y)
{
  U u{y};					// { dg-error "contract assertion cannot be proven" }
}
