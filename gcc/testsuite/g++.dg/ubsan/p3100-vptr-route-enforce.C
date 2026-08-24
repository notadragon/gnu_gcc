// P3100 Task 4.1 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=vptr the routed vptr check resolves to noexcept_enforce:
// the handler runs (kind 7, semantic 7) and the program then TERMINATES.

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
  // noexcept_enforce terminated inside the check -- this must NOT be reached.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// Handler ran with kind 7 (implicit) and semantic 7 (noexcept_enforce).
// Anchored at ^ so no stock UBSan text leaks before the handler; dg-shouldfail
// proves the program then terminated (so "survived" is never reached).
// { dg-output "^handler kind=7 semantic=7" }
