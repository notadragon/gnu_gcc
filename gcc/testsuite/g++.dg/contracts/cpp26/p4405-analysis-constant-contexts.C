// D4405 ("The Analysis Evaluation"): during constant evaluation there is no
// analysis evaluation, so a call in a manifestly constant-evaluated
// expression has none: the initializer of a constexpr or constinit
// variable (at namespace scope, in a function, a static data member), a
// static_assert condition, a template argument, an `if constexpr'
// condition, a case label, an enumerator, a bit-field width, and -- in a
// template -- each of those as it is instantiated; every call in an
// immediate function and in the consteval branch of `if consteval', and a
// call of an immediate function.  The constant evaluation evaluates the
// precondition, analyzed with diagnose, as observe: a warning at the
// precondition (DECISIONS.md E142).  A lambda's body defined in such an
// initializer is a function of its own, whose calls are analyzed, as is a
// run-time call.  (GCC-928: each of them was analyzed.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

constexpr int h (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-warning "contract predicate is false in constant expression" }
// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }

template<int N> struct A { static constexpr int value = N; };

constexpr int v1 = h (-1);			// { dg-bogus "always false|cannot be proven" }
constinit int v2 = h (-1);			// { dg-bogus "always false|cannot be proven" }
static_assert (h (-1) == -1);			// { dg-bogus "always false|cannot be proven" }
static_assert (A<h (-1)>::value == -1);		// { dg-bogus "always false|cannot be proven" }
enum E { e = h (-1) };				// { dg-bogus "always false|cannot be proven" }
struct B {
  static constexpr int s = h (-1);		// { dg-bogus "always false|cannot be proven" }
  int bits : h (-1) + 3;			// { dg-bogus "always false|cannot be proven" }
};

consteval int
ce (int x)
{
  return h (x);					// { dg-bogus "always false|cannot be proven" }
}

constexpr int
ci (int x)
{
  if consteval
    {
      return h (x);				// { dg-bogus "always false|cannot be proven" }
    }
  return 1;
}

int
f (int y)
{
  constexpr int l1 = h (-1);			// { dg-bogus "always false|cannot be proven" }
  if constexpr (h (-1) < 0)			// { dg-bogus "always false|cannot be proven" }
    y++;
  switch (y)
    {
    case h (-1):				// { dg-bogus "always false|cannot be proven" }
      return l1;
    }
  y += ce (-1);					// { dg-bogus "always false|cannot be proven" }
  y += ci (y);
  constexpr auto lam = [] (int z) { return h (z); };	// { dg-warning "contract assertion cannot be proven" }
  y += lam (y);
  return y + h (-1);				// { dg-warning "contract predicate is always false" }
}

template<typename T>
int
g (T y)
{
  constexpr int l2 = h (-1);			// { dg-bogus "always false|cannot be proven" }
  static_assert (h (-1) == -1);			// { dg-bogus "always false|cannot be proven" }
  if constexpr (A<h (-1)>::value < 0)		// { dg-bogus "always false|cannot be proven" }
    y++;
  return y + l2 + h (y);			// { dg-warning "contract assertion cannot be proven" }
}

int gi = g (1);
