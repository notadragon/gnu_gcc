// P4405 premises decided by this TU (T1-GCC), across translation units: a
// premise from a callee inlined into the real function is this TU's to decide.
// g inlines h, whose precondition its TU enforces, so g's diagnose evaluation
// relying on it is sound and assumed; the only out-of-line copy of h is another
// TU's, which ignores it, and the linker keeps it.  g still runs its own
// inlined check: g (-5) is a violation, which the handler ends successfully.
//
// Mirror: clang/test/Contracts/Runnable/p4405-premise-multi-tu-inlined.cpp
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-multi-tu-inlined.json" }
// { dg-additional-sources "p4405-premise-multi-tu-inlined-i.cc p4405-premise-multi-tu-inlined-e.cc" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

int g (int);

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int main ()
{
  g (-5);
  __builtin_abort ();
}

