// P3100 full-coverage UBSan routing: with noexcept_enforce a routed check runs
// the handler (kind 7, semantic 7) then terminates -- representative of the
// terminating direction for the full-coverage batch (the mechanism is shared
// across all routed libubsan checks).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=signed-integer-overflow -fsanitize-semantic=signed-integer-overflow:noexcept_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the handler" }

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
  volatile int a = 2147483647, b = 1;
  volatile int r = a + b;
  (void) r;
  std::printf ("survived\n");  // must NOT be reached
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=7" }
