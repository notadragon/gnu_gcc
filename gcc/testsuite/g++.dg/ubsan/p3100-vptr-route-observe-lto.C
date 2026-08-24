// P3100 Task 4.1 (UBSan runtime routing) under LTO: the routing descriptor is a
// preserved weak global and the recover-codegen selection for a continuing
// routed semantic is keyed on the STREAMED -fsanitize-semantic= option, so
// noexcept_observe still continues correctly at LTRANS with
// -fno-fat-lto-objects (where the C++-only -fcontracts-p3100 is not streamed).
// This is the highest-risk path: a non-recover LTRANS codegen would re-fault on
// the continue and (with the crash-state gate skipped) loop.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe -flto -fno-fat-lto-objects" }
// { dg-require-effective-target lto }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// Handler ran (kind 7, noexcept_observe 6) and the program CONTINUED under LTO.
// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
