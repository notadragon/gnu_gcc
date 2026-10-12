// Under P3595 dynamic selection, an enforced violation (a selector value
// with no arm) whose handler throws unwinds from its own site.
//
// Mirror: clang/test/Contracts/dyn-enforced-shared-block-eh.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/dyn-enforced-shared-block-eh.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

extern "C" std::contracts::evaluation_semantic my_sel() {
  return static_cast<std::contracts::evaluation_semantic>(9);   // no arm
}

static int dtor = 0, inner = 0, calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  if (++calls > 3)   // the misrouted throw loops back here
    std::exit(2);
  throw 1;
}

struct D { ~D() { ++dtor; } };

void g(bool first) {
  if (first) {
    try {
      D d;
      contract_assert(false);
    } catch (int) {
      ++inner;
    }
  }
  contract_assert(false);   // must unwind to main, not to the catch above
}

int main() {
  int outer = 0;
  try { g(true); } catch (int) { outer = 1; }
  if (inner != 1 || dtor != 1 || outer != 1)
    __builtin_abort();
}
