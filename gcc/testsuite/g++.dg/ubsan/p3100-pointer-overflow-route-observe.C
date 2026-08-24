// P3100 Task 4.1 (UBSan runtime routing): the pointer-overflow check
// (-fsanitize=pointer-overflow, ErrorType::PointerOverflow) routes to the
// contract-violation handler when pointer arithmetic overflows.  Recoverable,
// so with -fcontracts-p4298 the default resolves to noexcept_observe: handler
// runs, program CONTINUES.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=pointer-overflow" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

char *volatile sink;
char a[64];

char *__attribute__((noinline))
add (char *p, unsigned long i)
{
  return p + i;  // pointer-overflow when p + i wraps
}

int main ()
{
  char *p = &a[0];
  sink = add (p, -(unsigned long) p - 1);  // base overflows to (char *) -1
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
