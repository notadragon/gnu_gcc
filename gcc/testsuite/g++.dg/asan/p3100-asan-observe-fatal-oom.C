// With the address check routed as noexcept_observe, a report ASan builds as
// fatal -- its caller cannot continue past it -- runs the handler and then
// terminates, instead of returning.  Without the fix:
// An allocation-size-too-big report returns into malloc, which reports
// out-of-memory and returns null to code that expects success.
// Correct: the program terminates non-zero with at most one handler call.
// A second handler call exits 0.
//
// Mirror: clang/test/Contracts/Runnable/p3100-asan-observe-fatal-oom.cpp
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
  char *volatile b = static_cast<char *>(std::malloc (std::size_t (1) << 42));
  b[0] = 1;
  return 0;
}
