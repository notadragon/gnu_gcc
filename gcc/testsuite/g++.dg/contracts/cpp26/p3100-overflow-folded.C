// The P3100 signed-overflow check is lowered in pass_ubsan, after folding, so
// the folders must not assume that signed overflow cannot happen: `x + 1 > x'
// is not folded to true nor `x * 4 / 4' to x.  Each overflow is reported, and
// the result is the wrapped value.
//
// Mirror: clang/test/Contracts/Runnable/p3100-overflow-folded.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-overflow-folded.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <climits>

static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++calls;
}

__attribute__((noinline)) bool gt (int x) { return x + 1 > x; }
__attribute__((noinline)) int muldiv (int x) { return x * 4 / 4; }

int main ()
{
  if (gt (INT_MAX) || calls != 1)
    __builtin_abort ();
  if (muldiv (INT_MAX) != -1 || calls != 2)
    __builtin_abort ();
}
