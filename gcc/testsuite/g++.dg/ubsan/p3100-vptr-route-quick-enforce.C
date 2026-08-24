// P3100 Task 4.1 (UBSan runtime routing): -fsanitize-semantic=vptr:quick_enforce
// terminates WITHOUT calling the handler and WITHOUT any output -- the routed
// runtime Die()s silently on the vptr check.  quick_enforce needs no
// -fcontracts-p4298 (no handler is involved).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates on the violation" }

#include <contracts>
#include <cstdio>

// If the handler were (wrongly) invoked for quick_enforce it would print this;
// the dg-output assertion below proves it is NOT.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::printf ("handler ran\n");
  std::fflush (stdout);
}

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };

int __attribute__((noinline))
access_b (T *p)
{
  return p->b;  // vptr: member access; *p is really an S, not a T.
}

int main ()
{
  S s;
  T *p = reinterpret_cast<T *> (&s);
  volatile int sink = access_b (p);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// quick_enforce = silent terminate: NO handler, NO stock UBSan report, NO
// "survived".  Total output is empty.  (Non-vacuous: routing to the handler, or
// leaking the stock report, or continuing, each breaks this.)
// { dg-output "^\[ \t\r\n\]*$" }
