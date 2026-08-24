// P3100 Task 4.1 (UBSan runtime routing):
// -fsanitize-semantic=nonnull-attribute:quick_enforce terminates WITHOUT
// calling the handler and WITHOUT any output -- the routed runtime Die()s
// silently.  quick_enforce needs no -fcontracts-p4298.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize=nonnull-attribute -fsanitize-semantic=nonnull-attribute:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates on the violation" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::printf ("handler ran\n");  // must NOT run for quick_enforce
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
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// quick_enforce = silent terminate: empty output.
// { dg-output "^\[ \t\r\n\]*$" }
