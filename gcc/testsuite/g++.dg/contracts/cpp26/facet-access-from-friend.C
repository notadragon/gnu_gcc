// A private compute_semantic is not a facet -- the library concept is false
// for it -- wherever the contract is written: inside a friend of the label as
// at namespace scope, access is judged as the concept judges it, so the
// configured observe stays observe.
//
// Mirror: clang/test/Contracts/facet-access-from-friend.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
using std::contracts::evaluation_semantic;

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

struct L {
  using assertion_control_object = L;
private:
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::enforce; }
  friend struct User;
};
constexpr L lbl{};
static_assert (!std::contracts::labels::semantic_computation_label<L>);

struct User { static void f (int x) pre<lbl> (x > 0) {} };
void g (int x) pre<lbl> (x > 0) {}

int main ()
{
  g (-1);
  User::f (-1);
  return violations == 2 ? 0 : 1;
}
