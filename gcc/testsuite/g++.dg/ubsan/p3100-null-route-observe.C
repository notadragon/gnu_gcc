// P3100 full-coverage UBSan routing: the null check (load/store through a null
// pointer) routes to the handler.  Null dereference has no defined fallback:
// noexcept_observe calls the handler once (kind 7, semantic 6), and behavior
// after is unconstrained -- the ensuing real null load faults.  (null is also a
// native Group-B check; the native path is off unless configured.)

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "null dereference faults after the observed no-fallback violation" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

int main ()
{
  int *volatile p = nullptr;
  volatile int r = *p;  // load through null: UB, no defined fallback
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// Handler ran (the sanitizer emits nothing itself); the anchored ^ rejects any
// leaked pre-handler UBSan text.
// { dg-output "^handler kind=7 semantic=6" }
