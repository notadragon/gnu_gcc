// P3290: when a failed assert's contract-violation handler exits via an
// exception, the assert entry point catches it and terminates via
// std::abort() (NOT std::terminate()).  This is specific to the C assert
// integration: the paper requires abort() on *any* completion of the handler,
// so the entry point wraps the dispatch in try { ... } catch { std::abort(); }.
//
// Discrimination: a std::terminate handler that would _Exit(0) proves, via
// dg-shouldfail, that abort() (SIGABRT) is used rather than terminate() -- if
// terminate() ran, the program would exit 0 and fail dg-shouldfail.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "assert aborts rather than terminating, even on a throw" }

#include <cassert>
#include <contracts>
#include <cstdlib>
#include <exception>

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw 42;
}

[[noreturn]] void my_terminate() { std::_Exit(0); }

int main()
{
  std::set_terminate(my_terminate);
  assert(1 == 2);
  return 0;
}
