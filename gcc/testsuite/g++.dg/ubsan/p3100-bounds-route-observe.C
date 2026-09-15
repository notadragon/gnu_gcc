// P3100 full-coverage UBSan routing: the bounds check routes to the handler;
// noexcept_observe continues.  (bounds is also a native Group-B check; the
// native path is off unless configured, so the sanitizer routes cleanly here.)

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=bounds -fsanitize-semantic=bounds:noexcept_observe" }
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
  int a[3] = { 0, 0, 0 };
  volatile int i = 5;
  volatile int r = a[i];  // array subscript out of bounds: UB
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
