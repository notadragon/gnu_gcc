// P3100 Task 4.1 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=returns-nonnull-attribute the routed check resolves to
// noexcept_enforce: the handler runs (kind 7, semantic 7) then the program
// TERMINATES.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=returns-nonnull-attribute -fno-sanitize-recover=returns-nonnull-attribute" }
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

__attribute__((returns_nonnull)) int *__attribute__((noinline))
g ()
{
  int *volatile q = nullptr;
  return q;
}

int main ()
{
  volatile int *r = g ();
  (void) r;
  std::printf ("survived\n");  // must NOT be reached
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=7" }
