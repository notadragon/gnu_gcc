// A fold over a captured parameter pack in a lambda's contract predicate.
// The lambda's contracts are substituted (tsubst_lambda_expr) while its
// operator() is being defined, which is where an implicit capture of the
// pack's elements is made; that substitution injected a stand-in `this',
// which made it look like a default member initializer where nothing is
// captured, and it used a copy of the local specializations, so the body
// captured again (GCC-634, an ICE in expand_expr_real_1).  An explicit pack
// capture's proxy was never mapped to the instantiation's
// (reconstruct_lambda_capture_pack).  Stock GCC rejects these, then ICEs.
//
// Mirror: clang/test/Contracts/lambda-pre-fold-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

template <class... Ts>
int
implicit_copy (int v, Ts... ts)
{
  auto l = [=] (int u) pre (((u > ts) && ...)) { return (u + ... + ts); };
  return l (v);
}

template <class... Ts>
int
explicit_copy (int v, Ts... ts)
{
  auto l = [ts...] (int u) pre (((u > ts) && ...)) { return (u + ... + ts); };
  return l (v);
}

template <class... Ts>
int
by_reference (int v, Ts... ts)
{
  auto l = [&] (int u) pre (((u > ts) && ...)) post (r: ((r > ts) && ...))
    { return u + (0 * ... * ts); };
  return l (v);
}

template <class... Ts>
int
pre_only (int v, Ts... ts)
{
  // The pack is named only in the lambda's own precondition.
  auto l = [ts...] (int u) pre (((u > ts) && ...)) { return u; };
  return l (v);
}

int
main ()
{
  if (implicit_copy (5, 1, 2) != 8 || explicit_copy (5, 1, 2) != 8
      || by_reference (5, 1, 2) != 5 || pre_only (5, 1, 2) != 5)
    __builtin_abort ();
  if (violations != 0)
    __builtin_abort ();
  implicit_copy (2, 1, 2);
  explicit_copy (2, 1, 2);
  pre_only (2, 1, 2);
  if (violations != 3)
    __builtin_abort ();
  by_reference (2, 1, 2);
  if (violations != 5)
    __builtin_abort ();
}
