// P3100 Bug #1 (runtime guard for the recover-mode codegen fix): a routed ASan
// STACK-buffer-overflow under noexcept_observe must call the handler and
// CONTINUE.  The routed continue path requires recover-mode (_noabort) codegen;
// before the fix GCC emitted the non-recoverable (noreturn) report call and,
// because it kept the array base in a call-clobbered register across that
// "never returns" call, returning from it (as the routing runtime does for a
// continue semantic) ran the following load with a clobbered base -> SIGSEGV,
// then a libsanitizer kErrorKindInvalid CHECK failure.  Uses
// -fsanitize-semantic= (NOT -fsanitize-recover=) so it exercises the
// semantic-driven recover path specifically -- the sibling
// p3100-asan-route-observe.C passes -fsanitize-recover= and so did not catch
// this.  The heap sibling never crashed (its pointer landed in a register the
// report did not clobber); a stack array is needed to expose it.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize-semantic=address:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

volatile int sink;

int __attribute__((noinline))
oob (int i)
{
  int a[3] = { 1, 2, 3 };
  return a[i];			// stack out-of-bounds read when i is out of [0,3)
}

int main (int argc, char **)
{
  sink = oob (argc + 4);	// argc >= 1 -> i >= 5, not constant-foldable
  // noexcept_observe = report + continue: we must reach here after the fix.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
// { dg-output ".*survived" }
