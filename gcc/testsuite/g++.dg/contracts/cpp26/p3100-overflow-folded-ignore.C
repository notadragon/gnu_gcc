// Under `ignore' a P3100 signed overflow has the defined wrapped value, also in
// the shapes the folders would simplify by assuming it cannot happen:
// `x + 1 > x' is false and `x * 4 / 4' is -1 for INT_MAX.
//
// Mirror: clang/test/Contracts/Runnable/p3100-overflow-folded-ignore.cpp in
// the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow-folded-ignore.json" }

#include <climits>

__attribute__((noinline)) bool gt (int x) { return x + 1 > x; }
__attribute__((noinline)) int muldiv (int x) { return x * 4 / 4; }

int main ()
{
  if (gt (INT_MAX))
    __builtin_abort ();
  if (muldiv (INT_MAX) != -1)
    __builtin_abort ();
}
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
