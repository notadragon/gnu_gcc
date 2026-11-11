// P3400: a label that does not depend on the template parameters, on a
// contract assertion of a function template, is resolved where the
// template is declared, and its allowed_semantics facet is a constant
// expression there too.  It used to be rejected as "must be usable in a
// constant expression" (GCC-916).  Each label restricts the semantics as
// it does on a function that is not a template: constant evaluation with
// the configured observe finds the predicates false.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct my_enforce_t {
  using assertion_control_object = my_enforce_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::enforce};
};
constexpr my_enforce_t my_enforce{};

template<typename T>
constexpr T e (T x) pre<my_enforce> (x > 0) { return x; }	// { dg-error "contract predicate is false" }

template<typename T>
constexpr T t (T x) pre<std::contracts::labels::terminating> (x > 0)	// { dg-error "contract predicate is false" }
{
  contract_assert<my_enforce> (x != 2);	// { dg-error "contract predicate is false" }
  return x;
}

template<typename T>
T d (T x) pre<my_enforce> (x > 0);	// { dg-bogus "constant expression" }

constexpr int a = e (1);
constexpr int b = e (-1);
constexpr int c = t (2);
constexpr int f = t (-1);
int g (int x) { return d (x); }
