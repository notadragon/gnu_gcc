// P3100 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=vptr the reference form of a static_cast downcast
// (ub:expr.static.cast.base.class) resolves to noexcept_enforce: the handler
// runs (kind 7, semantic 7) and the program terminates.

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

// { dg-output "^handler kind=7 semantic=7" }
