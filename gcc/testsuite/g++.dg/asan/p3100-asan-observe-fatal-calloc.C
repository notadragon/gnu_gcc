// With the address check routed as noexcept_observe, a report ASan builds as
// fatal -- its caller cannot continue past it -- runs the handler and then
// terminates, instead of returning.  Without the fix:
// A calloc-overflow report returns into calloc, which allocates the wrapped
// size (C requires a null result).
// Correct: the program terminates non-zero with at most one handler call.
// A second handler call exits 0.
//
// Mirror: clang/test/Contracts/Runnable/p3100-asan-observe-fatal-calloc.cpp
// in the llvm_llvm-project fork.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=address:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "fatal ASan report" }

#include <contracts>
#include <cstdlib>

static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  if (++calls > 1)
    std::_Exit (0);
}

int main ()
{
  volatile std::size_t n = std::size_t (1) << 33, s = std::size_t (1) << 31;
  void *volatile p = std::calloc (n, s);  // volatile: the call is not elided
  std::free (p);
  if (!p)
    return 1; // null is also correct
  return 0;
}
