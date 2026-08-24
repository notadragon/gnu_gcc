// P3100 Task 4.1 (UBSan runtime routing): the returns-nonnull-attribute check
// (-fsanitize=returns-nonnull-attribute, ErrorType::InvalidNullReturn) routes to
// the contract-violation handler when a function marked
// __attribute__((returns_nonnull)) returns a null pointer.  Recoverable, so with
// -fcontracts-p4298 the default resolves to noexcept_observe: handler runs,
// program CONTINUES.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=returns-nonnull-attribute" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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
  return q;  // returns-nonnull-attribute: null returned from returns_nonnull
}

int main ()
{
  volatile int *r = g ();
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
