// [basic.contract.eval]: the preconditions of a function are evaluated in
// order, left to right, before its body, and its postconditions after it.
// The base facility's order at run time; p3097-order.C pins the order of a
// virtual call with P3097.
//
// Mirror: clang/test/Contracts/Runnable/evaluation-order.cpp in the
// llvm_llvm-project fork.

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

char order[8];
int n = 0;
bool rec (char c) { order[n++] = c; return true; }

int f ()
  pre (rec ('a'))   // first precondition
  pre (rec ('b'))   // second precondition (evaluated after the first)
  post (rec ('d'))  // postcondition (evaluated after the body)
{
  rec ('c');        // body
  return 0;
}

int
main ()
{
  f ();
  order[n] = '\0';
  if (__builtin_strcmp (order, "abcd") != 0)
    __builtin_abort ();
  return 0;
}
