// P3098: Capture init exception, quick_enforce -- terminates directly.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "terminate" }

#include <contracts>
#include <cstdlib>

// quick_enforce never calls the handler: a call is a failure, and
// exiting normally is how a dg-shouldfail test reports one.
void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

struct Bad {
  Bad(int) { throw 42; }
  ~Bad() {}
};

int f(int i)
  post [b = Bad(1)] (true)
{
  return i;
}

int main() {
  f(10);
  return 1;
}
