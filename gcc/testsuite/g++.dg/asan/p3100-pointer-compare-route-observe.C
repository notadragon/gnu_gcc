// P3100 Task 4.1: the ASan pointer-compare check (comparing two pointers into
// different objects -- [expr.rel] UB) is routed to the contract-violation
// handler.  These pointer-pair checks report through the SAME ASan
// ScopedInErrorReport path as address errors, but via their OWN wire byte
// (__asan_contract_semantic_pointer_compare), so the address routing scope is
// unchanged.  With -fcontracts-p4298 and -fsanitize-recover=pointer-compare the
// check resolves to noexcept_observe: the handler runs and the program
// CONTINUES.
//
// The check only detects when the runtime flag detect_invalid_pointer_pairs is
// set, so ASAN_OPTIONS enables it here.

// { dg-do run }
// { dg-set-target-env-var ASAN_OPTIONS "detect_invalid_pointer_pairs=2:halt_on_error=0" }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=pointer-compare -fsanitize-recover=pointer-compare" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>
#include <cstdlib>

// Non-throwing handler: a throwing handler would std::terminate() from inside
// libasan's noexcept ScopedInErrorReport destructor, which is why the routed
// semantic is noexcept_observe.  The comment names the specific check.
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

volatile int sink;

int __attribute__((noinline))
cmp (char *p, char *q)
{
  return p < q;  // comparing pointers into different objects: [expr.rel] UB
}

int main ()
{
  char *heap1 = (char *) std::malloc (42);
  char *heap2 = (char *) std::malloc (42);
  sink = cmp (heap1, heap2);
  std::free (heap1);
  std::free (heap2);
  // noexcept_observe = report + continue: we reach here after the violation.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// The handler ran, reporting an implicit contract assertion (kind 7) with the
// NOEXCEPT_OBSERVE evaluation semantic (6).  RF5: the sanitizer emits NOTHING
// on the routed path, so the handler line is the FIRST output -- anchor at ^ so
// any leaked pre-handler sanitizer text would fail the match.
// { dg-output "^handler kind=7 semantic=6" }
// noexcept_observe continues: "survived" is printed after the detected compare.
// { dg-output ".*survived" }
