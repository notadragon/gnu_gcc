// P3098 x P3400 single-unit rule: under the enforce semantic the capture is
// constructed, the predicate is evaluated, and a failure terminates.
// { dg-do run { target c++26 } }
// { dg-shouldfail "enforced capturing postcondition violation terminates" }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

static int made(int v) { return v; }

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::fputs ("contract handler ran\n", stderr);
}

int f() post [old = made(5)] (r: r == old) { return 7; }  // 7 == 5 -> fails -> terminate

int main() {
  return f();  // should not return normally
}

// The handler runs before termination (a native trap would not print this).
// { dg-output "contract handler ran" }
