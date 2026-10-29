// P3100 full-coverage UBSan routing: the float-divide-by-zero check routes to
// the handler; noexcept_observe continues (the IEEE inf/nan result is a defined
// value to carry on with).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=float-divide-by-zero -fsanitize-semantic=float-divide-by-zero:noexcept_observe" }
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
  volatile double a = 1.0, b = 0.0;
  volatile double r = a / b;  // float divide by zero
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
