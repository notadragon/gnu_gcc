// Under observe, a speculative evaluation that is not manifestly constant
// reports no contract violation.
//
// Mirror: clang/test/Contracts/observe-warning-unused-var-eval.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }

struct N { bool r = [] (int x) pre (x > 5) { return true; } (1); };

void use ()
{
  N n;
}
