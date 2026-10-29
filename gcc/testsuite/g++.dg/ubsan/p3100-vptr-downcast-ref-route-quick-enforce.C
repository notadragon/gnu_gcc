// P3100 (UBSan runtime routing): -fsanitize-semantic=vptr:quick_enforce
// terminates on the reference form of a static_cast downcast
// (ub:expr.static.cast.base.class) without calling the handler and without
// any output.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "quick_enforce terminates on the violation" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::printf ("handler ran\n");
  std::fflush (stdout);
}

struct S { virtual ~S () {} int s = 1; };
struct T : S { int t = 2; };
struct U : S { int u = 3; };

__attribute__((noipa)) int lref (S &r) { return static_cast<T &> (r).s; }

int main ()
{
  U u;
  volatile int sink = lref (u);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^\[ \t\r\n\]*$" }
