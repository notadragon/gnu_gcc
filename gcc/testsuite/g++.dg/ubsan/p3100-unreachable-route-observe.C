// P3100 full-coverage UBSan routing: the unreachable check (reaching
// __builtin_unreachable) routes to the handler.  There is nowhere to continue
// to, so even noexcept_observe calls the handler once (kind 7, semantic 6) and
// then the noreturn report leg terminates.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=unreachable -fsanitize-semantic=unreachable:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "unreachable terminates after the handler" }

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
  volatile int z = 0;
  if (z)
    return 0;
  __builtin_unreachable ();  // reached: UB
}

// { dg-output "^handler kind=7 semantic=6" }
