// P3100 full-coverage UBSan routing: the integer-divide-by-zero check routes to
// the handler.  Division by zero has no defined fallback: noexcept_observe
// calls the handler once (kind 7, semantic 6), then the real division faults.
// (Also a native Group-B check; native path off unless configured.)

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=integer-divide-by-zero -fsanitize-semantic=integer-divide-by-zero:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "the division faults after the observed no-fallback violation" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

int main ()
{
  volatile int a = 1, b = 0;
  volatile int r = a / b;  // integer divide by zero: UB, no defined fallback
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
