// P3100 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=vptr a member function call through a glvalue whose
// type is not similar to the object's (ub:expr.ref.member.not.similar,
// polymorphic) resolves to noexcept_enforce: the handler runs (kind 7,
// semantic 7) and the program terminates.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fno-sanitize-recover=vptr" }
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

// { dg-output "^handler kind=7 semantic=7" }
