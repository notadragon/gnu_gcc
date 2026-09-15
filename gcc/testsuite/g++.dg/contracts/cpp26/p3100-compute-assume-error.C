// P3100: a compute_semantic result of "assume" that is not in the allowed set
// is an error (not a silent downgrade).  Here -fcontracts-allow-assume is not
// given, so assume is not in the set.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

// compute_semantic forces "assume", which is not in the allowed set.
struct to_assume_t {
  using assertion_control_object = to_assume_t;
  constexpr evaluation_semantic compute_semantic(evaluation_semantic) const
  { return evaluation_semantic::assume; }
};
constexpr to_assume_t to_assume{};

void f(int x)
  pre<to_assume>(x > 0)  // { dg-error "compute_semantic. result is not in the allowed" }
{
}
