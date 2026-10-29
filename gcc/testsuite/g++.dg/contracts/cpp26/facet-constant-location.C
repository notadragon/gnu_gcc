// The error for a compute_semantic facet that is not usable in a constant
// expression is reported at the contract, not wherever the parser or the
// instantiation happens to be when the semantic is resolved (the closing
// brace of the enclosing function, or the declarator-id of a function
// template at instantiation).
//
// Mirror: clang/test/Contracts/facet-constant-location.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>
using std::contracts::evaluation_semantic;

struct L {
  using assertion_control_object = L;
  evaluation_semantic compute_semantic (evaluation_semantic s) const
  { return s; }
};
constexpr L l{};

void g ()
{
  contract_assert<l> (true); // { dg-error "must be usable in a constant expression" }
} // { dg-bogus "must be usable in a constant expression" }

template <class T>
void t (T x) // { dg-bogus "must be usable in a constant expression" }
  pre<l> (x > 0) // { dg-error "must be usable in a constant expression" }
{ }

void h () { t (1); }
