// The outlined .pre function is marked noexcept (P4298) only if the
// postconditions whose captures it initializes are statically nonthrowing
// too: a throwing capture copy reaches the violation handler, and the
// handler's exception propagates to the caller.
//
// Mirror: clang/test/Contracts/outlined-pre-fn-noexcept-capture.cpp in the
// llvm_llvm-project fork (Clang has no outlined mode).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p4298 -fcontract-checks-outlined -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct Boom {
  int v;
  Boom (int i) : v (i) {}
  Boom (const Boom &) { throw 42; }
};

static int handled = 0;
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.detection_mode ()
      == std::contracts::detection_mode::evaluation_exception)
    ++handled;
  throw 7;
}

int f (const Boom b) post [c = b] (c.v > 0) { return b.v; }

int main ()
{
  try { f (Boom (1)); }
  catch (int e) { return (e == 7 && handled == 1) ? 0 : 1; }
  return 1;
}
