// P3100 full-coverage UBSan routing: the builtin check (invalid builtin use,
// e.g. __builtin_ctz(0)) routes to the handler; noexcept_observe continues.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=builtin -fsanitize-semantic=builtin:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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
  volatile unsigned z = 0;
  volatile int r = __builtin_ctz (z);  // ctz(0): UB
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
