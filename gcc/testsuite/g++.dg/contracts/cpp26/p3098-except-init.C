// P3098: Exception during capture initialization.
// With observe semantic: handler is called with post_capture kind and
// evaluation_exception detection mode.  Handler returns, execution continues,
// predicate is skipped.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdlib>

struct Bad {
  Bad(int) { throw 42; }
  Bad(const Bad&) { throw 42; }
  ~Bad() {}
};

int f(int i)
  post [b = Bad(1)] (true)
{
  return i;
}

int main() {
  int result = f(10);
  // If we reach here, the handler returned and execution continued.
  if (result != 10)
    __builtin_abort ();
}
// { dg-output "contract violation in function int f.int. at .*(\n|\r\n|\r)" }
// { dg-output ".assertion_kind: post_capture, semantic: observe, mode: evaluation_exception.*terminating: no.*(\n|\r\n|\r)" }
