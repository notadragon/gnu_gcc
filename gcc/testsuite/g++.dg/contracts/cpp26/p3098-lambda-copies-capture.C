// A lambda in a postcondition's predicate captures one of the
// postcondition's captures (P3098).  A capture of a function defined outside
// a class has no DECL_CONTEXT, so outer_automatic_var_p did not see that the
// lambda names it from outside, the closure did not capture it, and its
// operator() referred to the variable directly (an ICE in
// expand_expr_real_1, GCC-640).
//
// Mirrors: clang/test/Contracts/Runnable/p3098-lambda-copies-capture.cpp in
// the llvm_llvm-project fork, and p3098-mutable-lambda-copies-capture.cpp
// there for `k', `k2' and `k3' (CLANG-643; GCC-662).  The compile-time half,
// including a non-mutable lambda's const copy, is
// p3098-mutable-lambda-copies-capture.C.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

// (Not on a declaration followed by a separate definition: the capture's
// initializer then reads the declaration's parameter, GCC-658.)
int g (int y) post [old = y] ([=] { return old > 0; } ()) { return y; }
int h (int y) post [old = y] ([&] { return old > 0; } ()) { return y; }
int k (int y) post [old = y] ([=] () mutable { return ++old > 1; } ()) { return y; }
// The capture is unchanged by the mutable lambda's copy.
int k2 (const int y) post [old = y] ([=] () mutable { return ++old > 1; } () && old == y)
{ return y; }
// A nested mutable lambda may modify its copy of the outer lambda's copy.
int k3 (int y)
  post [old = y] ([=] () mutable { return [=] () mutable { return ++old; } () > 1; } ())
{ return y; }
auto lam = [] (int y) post [old = y] ([=] { return old > 0; } ()) { return y; };
struct S
{
  int m (int y) post [old = y] ([=] { return old > 0; } ()) { return y; }
};
template <class T>
T t (T y) post [old = y] ([=] { return old > 0; } ()) { return y; }

int
main ()
{
  if (g (1) + h (1) + k (1) + k2 (1) + k3 (1) + lam (1) + S ().m (1) + t (1)
      != 8)
    __builtin_abort ();
  if (violations != 0)
    __builtin_abort ();
  g (0);
  h (0);
  k (0);
  k2 (0);
  k3 (0);
  lam (0);
  S ().m (0);
  t (0);
  if (violations != 8)
    __builtin_abort ();
}
