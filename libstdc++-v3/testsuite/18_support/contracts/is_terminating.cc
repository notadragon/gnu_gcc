// { dg-options "-fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
// { dg-do run { target c++26 } }

// contract_violation::is_terminating.  observe continues, so it is false
// here; the terminating semantics are covered from g++.dg, where the
// program can be allowed to die.

#include <contracts>
#include <testsuite_hooks.h>

static int calls = 0;

void handle_contract_violation(const std::contracts::contract_violation& v)
{
  ++calls;
  VERIFY( !v.is_terminating() );
  VERIFY( v.semantic() == std::contracts::evaluation_semantic::observe );
  VERIFY( !std::contracts::is_nonthrowing(v.semantic()) );
}

void f(int i) pre (i > 10) { }

int main()
{
  f(0);
  VERIFY( calls == 1 );
}
