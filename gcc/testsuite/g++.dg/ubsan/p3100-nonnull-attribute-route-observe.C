// P3100 Task 4.1 (UBSan runtime routing): the nonnull-attribute check
// (-fsanitize=nonnull-attribute, ErrorType::InvalidNullArgument) routes to the
// contract-violation handler when a null pointer is passed to a parameter marked
// __attribute__((nonnull)).  Recoverable, so with -fcontracts-p4298 the default
// resolves to noexcept_observe: handler runs, program CONTINUES.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=nonnull-attribute" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

// The nonnull attribute is on the FUNCTION: argument 1 must be non-null.
void __attribute__((noinline, nonnull (1)))
g (int *p)
{
  (void) p;
}

int main ()
{
  int *volatile q = nullptr;
  g (q);  // nonnull-attribute: null passed to a nonnull parameter
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
