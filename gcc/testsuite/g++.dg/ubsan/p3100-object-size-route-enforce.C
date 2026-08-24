// P3100 Task 4.1 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=object-size the routed object-size check resolves to
// noexcept_enforce: the handler runs (kind 7, semantic 7) then the program
// TERMINATES.  object-size only instruments at -O1+; restrict to -O2.

// { dg-do run }
// object-size needs __builtin_object_size, so it only instruments at -O1+.
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=object-size -fno-sanitize-recover=object-size -O2" }
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

static char buf[1] alignas (int);

int main ()
{
  int *p = reinterpret_cast<int *> (&buf[0]);
  volatile int sink = *p;
  (void) sink;
  std::printf ("survived\n");  // must NOT be reached
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=7" }
