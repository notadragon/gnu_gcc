// P3100 Task 4.1 (UBSan runtime routing): the object-size check
// (-fsanitize=object-size, ErrorType::InsufficientObjectSize) routes to the
// contract-violation handler.  object-size is recoverable, so with
// -fcontracts-p4298 the default resolves to noexcept_observe: the handler runs
// and the program CONTINUES.  object-size needs __builtin_object_size, so it is
// only emitted at -O1+; restrict the run to -O2.

// { dg-do run }
// object-size needs __builtin_object_size, so it only instruments at -O1+.
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=object-size -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

static char buf[1] alignas (int);  // aligned, but too small for an int

int main ()
{
  int *p = reinterpret_cast<int *> (&buf[0]);
  volatile int sink = *p;  // object-size: 4-byte access on a 1-byte object
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
