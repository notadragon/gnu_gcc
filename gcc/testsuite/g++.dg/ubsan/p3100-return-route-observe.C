// P3100 full-coverage UBSan routing: the return check (flowing off the end of a
// value-returning function) routes to the handler.  No value can be
// synthesized, so noexcept_observe calls the handler once (kind 7, semantic 6)
// and then the noreturn report leg terminates.  (Also a native Group-B check;
// native path off unless configured.)

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=return -fsanitize-semantic=return:noexcept_observe -Wno-return-type" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "missing return terminates after the handler" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

int __attribute__((noinline))
ff (bool b)
{
  if (b)
    return 1;
}  // flows off the end when b is false: UB

int main ()
{
  volatile int r = ff (false);
  (void) r;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// { dg-output "^handler kind=7 semantic=6" }
