// P3595: caller-side dynamic selection keyed by descriptor -- two call sites
// (distinguished by caller.location) resolve to different selectors.  Each
// site's wrapper must dispatch to its own selector; this would fail if
// wrapper_tuples_equal ignored the descriptor and the two sites collapsed
// onto one shared wrapper.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-dyn-distinct.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
using std::contracts::evaluation_semantic;

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++fired; }

// Site A's selector: observe -> handler called.
evaluation_semantic p3595_caller_sel_a() { return evaluation_semantic::observe; }
// Site B's selector: ignore -> handler not called.
evaluation_semantic p3595_caller_sel_b() { return evaluation_semantic::ignore; }

int f(int x) pre(x > 0) { return x; }   // callee-side ignored (CLI default)

int call_a() { return f(-1); }          // line 23: site A -> sel_a -> observe
int call_b() { return f(-1); }          // line 24: site B -> sel_b -> ignore

int main() {
  call_a();
  int after_a = fired;
  call_b();
  // site A fired, site B did not: the two wrappers stayed distinct.
  if (after_a != 1 || fired != 1) __builtin_abort();
}
