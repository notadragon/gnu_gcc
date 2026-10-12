// P3100 full-coverage UBSan routing: the enum check (loading an enum with an
// out-of-range value) routes to the handler; noexcept_observe continues.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=enum -fsanitize-semantic=enum:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

enum E { A, B };

int main ()
{
  int i = 7;
  E volatile e = *reinterpret_cast<E *> (&i);  // invalid enum load: UB
  (void) e;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
