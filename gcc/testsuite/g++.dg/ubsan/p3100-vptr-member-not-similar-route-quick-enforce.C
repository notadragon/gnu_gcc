// P3100 (UBSan runtime routing): -fsanitize-semantic=vptr:quick_enforce
// terminates on a member function call through a glvalue whose type is not
// similar to the object's (ub:expr.ref.member.not.similar, polymorphic)
// without calling the handler and without any output.

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

struct A { virtual ~A () {} int i = 1; };
struct B { virtual ~B () {} int j = 2; int f () { return 30; } };

__attribute__((noipa)) int call (A &a) { return reinterpret_cast<B &> (a).f (); }

int main ()
{
  A a;
  volatile int sink = call (a);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^\[ \t\r\n\]*$" }
