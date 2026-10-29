// P3100 (UBSan runtime routing): the routed report of a member access and of
// a member function call through a glvalue whose type is not similar to the
// object's (ub:expr.ref.member.not.similar, polymorphic) carries UBSan's
// "member access within" and "member call on" diagnostics, rendered on demand
// by contract_violation::report() and never printed by the sanitizer itself.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4301 -fsanitize=vptr" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  const char *r = v.report ();
  std::printf ("REPORT-BEGIN\n%s\nREPORT-END\n", r ? r : "(null)");
  std::fflush (stdout);
}

struct A { virtual ~A () {} int i = 1; };
struct B { virtual ~B () {} int j = 2; int f () { return 30; } };

__attribute__((noipa)) int dot (A &a) { return reinterpret_cast<B &> (a).j; }
__attribute__((noipa)) int call (A &a) { return reinterpret_cast<B &> (a).f (); }

int main ()
{
  A a;
  dot (a);
  call (a);
  return 0;
}

// { dg-output "^REPORT-BEGIN(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*p3100-vptr-member-not-similar-report.C:24:\[0-9]*: runtime error: member access within address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'.*REPORT-END(\n|\r\n|\r)" }
// { dg-output "REPORT-BEGIN(\n|\r\n|\r)" }
// { dg-output "\[^\n\r]*p3100-vptr-member-not-similar-report.C:25:\[0-9]*: runtime error: member call on address 0x\[0-9a-fA-F]* which does not point to an object of type 'B'.*REPORT-END" }
