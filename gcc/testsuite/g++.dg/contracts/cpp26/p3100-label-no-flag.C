// P3100: without -fcontracts-allow-assume, a label that lists "assume" in its
// allowed_semantics cannot re-introduce it -- the flag gate is authoritative.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=assume" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// Explicitly lists observe AND assume, but -fcontracts-allow-assume is off.
struct ob_as_t {
  using assertion_control_object = ob_as_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::observe, evaluation_semantic::assume};
};
constexpr ob_as_t ob_as{};

static int side = 0;
static int viol = 0;
bool chk(int x) { ++side; return x > 0; }
void handle_contract_violation(const std::contracts::contract_violation&)
{ ++viol; }

// The flag is off, so assume is not in the gated base and is intersected away:
// the set is {observe}.  Configured assume -> observe.
void f(int x) pre<ob_as>(chk(x)) {}

int main()
{
  f(-1);
  if (side != 1 || viol != 1) __builtin_abort();   // observe, not assume/ignore
}
