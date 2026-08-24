// P3100 Task 4.1 (UBSan runtime routing):
// -fsanitize-semantic=alignment:quick_enforce terminates WITHOUT calling the
// handler and WITHOUT any output -- the routed runtime Die()s silently on the
// alignment check.  quick_enforce needs no -fcontracts-p4298 (no handler).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize=alignment -fsanitize-semantic=alignment:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates on the violation" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::printf ("handler ran\n");  // must NOT run for quick_enforce
  std::fflush (stdout);
}

int __attribute__((noinline))
load (int *p)
{
  return *p;
}

int main ()
{
  alignas (int) char buf[8];
  int *p = reinterpret_cast<int *> (buf + 1);
  volatile int sink = load (p);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// quick_enforce = silent terminate: NO handler, NO stock UBSan report, NO
// "survived".  Total output is empty.
// { dg-output "^\[ \t\r\n\]*$" }
