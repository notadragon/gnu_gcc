// P3100 Task 4.1 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=nonnull-attribute the routed nonnull-attribute check
// resolves to noexcept_enforce: the handler runs (kind 7, semantic 7) then the
// program TERMINATES.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=nonnull-attribute -fno-sanitize-recover=nonnull-attribute" }
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

void __attribute__((noinline, nonnull (1)))
g (int *p)
{
  (void) p;
}

int main ()
{
  int *volatile q = nullptr;
  g (q);
  std::printf ("survived\n");  // must NOT be reached
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=7" }
