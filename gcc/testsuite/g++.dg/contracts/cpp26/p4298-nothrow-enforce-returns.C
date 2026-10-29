// Under -fcontracts-p4298, handle_enforced_contract_violation (std::nothrow,
// ...) reports noexcept_enforce, and a handler that returns is followed by
// abort: the [[noreturn]] function does not return.
//
// Mirror: clang/test/Contracts/p4298-nothrow-enforce-returns.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3290 -fcontracts-p4298" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <csignal>
#include <cstdlib>
#include <new>

void handle_contract_violation (const std::contracts::contract_violation &) {}

extern "C" void on_abort (int) { std::_Exit (0); }

__attribute__ ((noinline)) void g ()
{
  std::contracts::handle_enforced_contract_violation (std::nothrow, "manual");
}

int main ()
{
  std::signal (SIGABRT, on_abort);
  g ();
  return 1;  // a [[noreturn]] function returned
}
// { dg-require-effective-target signal }
