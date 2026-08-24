// P3400: a label's compute_semantic facet must still apply when the label
// is used on a templated contract.  Regression test: tsubst_contract never
// substituted CONTRACT_LABEL when instantiating a template, so grok_contract
// (which only resolves label facets for a non-dependent label) never saw a
// concrete label for any templated contract -- every facet (compute_semantic,
// allowed_semantics, custom handlers/messages) was silently dropped.  Here
// the label unconditionally forces "observe"; under a global "enforce"
// default, a non-template contract with this label correctly downgrades to
// observe (handler runs, no termination) -- the templated contract used to
// terminate instead, ignoring the label entirely.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct fixed_observe_t {
  using assertion_control_object = fixed_observe_t;
  constexpr evaluation_semantic
  compute_semantic (evaluation_semantic) const
  {
    return evaluation_semantic::observe;
  }
};

constexpr fixed_observe_t non_template_label{};

template<typename T>
constexpr fixed_observe_t dep_label{};

static int handler_count = 0;
void handle_contract_violation (const std::contracts::contract_violation&)
{
  ++handler_count;
}

void non_template_check (int x) pre<non_template_label> (x > 0) { }

template<typename T>
void template_check (T x) pre<dep_label<T>> (x > 0) { }

int main ()
{
  non_template_check (-1);
  if (handler_count != 1)
    __builtin_abort ();

  template_check<int> (-1);
  if (handler_count != 2)
    __builtin_abort ();

  return 0;
}
