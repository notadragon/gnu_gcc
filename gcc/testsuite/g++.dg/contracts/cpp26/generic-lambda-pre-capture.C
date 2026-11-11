// A generic lambda's contract predicate naming one of its captures.  A
// specialization of its operator() has its contracts substituted before its
// body, which declares the specialization's own capture proxies, so the
// pattern's proxy had nothing to map to and substitution fell into the
// "parameter used in a late-specified return type" recovery and its
// assertion (tsubst_expr, GCC-633; stock GCC too).  The predicate now names
// the closure member the proxy stands for.
//
// Mirror: clang/test/Contracts/generic-lambda-pre-capture.cpp in the
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

void
check (int n)
{
  if (violations != n)
    __builtin_abort ();
  violations = 0;
}

template <class T>
int
in_template (T t)
{
  auto l = [t] (auto u) -> int pre (u > t) { return u + t; };
  auto m = [&t] (auto u) pre (u > t) post (r: r > t) { return u + t; };
  return l (2) + m (2);
}

int
main ()
{
  int t = 1;
  auto copy = [t] (auto u) pre (u > t) { return u + t; };
  auto dflt = [=] (auto u) pre (u > t) { return u + t; };
  auto trailing = [t] (auto u) -> int pre (u > t) { return u + t; };
  auto init = [s = t] (auto u) -> int pre (u > s) { return u + s; };
  auto ref = [&t] (auto u) -> int pre (u > t) { return u + t; };
  auto post = [=] (auto u) -> int pre (u > t) post (r: r > t) { return u + 0 * t; };

  if (copy (2) + dflt (2) + trailing (2) + init (2) + ref (2) + post (2) != 17)
    __builtin_abort ();
  check (0);
  copy (0);
  dflt (0);
  trailing (0);
  init (0);
  ref (0);
  check (5);
  post (0);
  check (2);

  // By reference: the predicate sees the variable's current value.
  t = 5;
  ref (2);
  check (1);
  t = 1;

  in_template (1);
  check (0);
  in_template (3);
  check (2);
}
