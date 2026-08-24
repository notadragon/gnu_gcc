// P3100 Task 4.1: the ASan pointer-subtract check (subtracting two pointers
// into different objects -- expr.add.sub.diff.pointers UB) is routed to the
// contract-violation handler via its OWN wire byte
// (__asan_contract_semantic_pointer_subtract), independent of the address
// routing scope.  With -fcontracts-p4298 and -fsanitize-recover=pointer-subtract
// the check resolves to noexcept_observe: the handler runs and the program
// CONTINUES.  detect_invalid_pointer_pairs must be enabled at runtime.

// { dg-do run }
// { dg-set-target-env-var ASAN_OPTIONS "detect_invalid_pointer_pairs=2:halt_on_error=0" }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=pointer-subtract -fsanitize-recover=pointer-subtract" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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
  // noexcept_observe = report + continue: we reach here after the violation.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The handler ran (kind 7) with the NOEXCEPT_OBSERVE evaluation semantic (6).
// { dg-output "^handler kind=7 semantic=6" }
// noexcept_observe continues: "survived" is printed after the detected subtract.
// { dg-output ".*survived" }
