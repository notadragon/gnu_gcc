// Where a P3100 implicit check and the sanitizer check are handled at the same
// place, the contract configuration wins; the sanitizer checks only what no
// contract covers.  Here for signed overflow: the configuration selects
// noexcept_enforce, the sanitizer is routed at noexcept_observe, and the
// handler exits 0 only when it sees the configuration's semantic.
//
// Mirror: clang/test/Contracts/p3100-precedence-overflow.cpp in the
// llvm_llvm-project fork.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=signed-integer-overflow -fsanitize-semantic=signed-integer-overflow:noexcept_observe" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/ubsan/p3100-precedence.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <climits>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  // Exit 0 only for the configuration's semantic.
  std::_Exit (v.semantic ()
	      == std::contracts::evaluation_semantic::noexcept_enforce ? 0 : 1);
}

__attribute__((noinline)) int op (int x) { return x + 1; }

int main ()
{
  volatile int r = 0;
  r = op (INT_MAX);
  return 2;  // no handler call at all
}
