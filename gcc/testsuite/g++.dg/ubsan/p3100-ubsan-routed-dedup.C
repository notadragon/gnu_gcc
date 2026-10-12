// A UBSan check routed to the contract-violation handler calls the handler for
// every violation, not only the first at a source location: UBSan's
// per-location dedup (SourceLocation::acquire, ignoreReport) does not apply to
// a routed report.
//
// Mirror: clang/test/Contracts/Runnable/p3100-ubsan-routed-dedup.cpp in the
// llvm_llvm-project fork, which also checks that a suppressions file does not
// drop a routed report.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=signed-integer-overflow -fsanitize-semantic=signed-integer-overflow:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <climits>

static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++calls;
}

__attribute__((noinline)) int add (int x) { return x + 1; }

int main ()
{
  for (int i = 0; i < 3; ++i)
    add (INT_MAX);
  if (calls != 3)
    __builtin_abort ();
}
