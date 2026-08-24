// P3100 Task 4.1: the ASan pointer-subtract check (expr.add.sub.diff.pointers
// UB) routed to the contract-violation handler.  With -fcontracts-p4298 and the
// default (no -fsanitize-recover=), the check resolves to noexcept_enforce: the
// handler runs, then the program TERMINATES.

// { dg-do run }
// { dg-set-target-env-var ASAN_OPTIONS "detect_invalid_pointer_pairs=2:halt_on_error=1" }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=pointer-subtract" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after routing the pointer-subtract report" }

#include <contracts>
#include <cstdio>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

volatile long sink;

long __attribute__((noinline))
sub (char *p, char *q)
{
  return p - q;  // subtracting pointers into different objects: UB
}

int main ()
{
  char *heap1 = (char *) std::malloc (42);
  char *heap2 = (char *) std::malloc (42);
  sink = sub (heap1, heap2);
  std::free (heap1);
  std::free (heap2);
  // noexcept_enforce = report + terminate: we must NOT reach here.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The handler ran (kind 7) with the NOEXCEPT_ENFORCE evaluation semantic (7).
// { dg-output "^handler kind=7 semantic=7" }
// noexcept_enforce terminates (dg-shouldfail): "survived" is never printed.
