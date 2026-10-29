// D4405R0, "Local Proofs": unoptimized, only folding proves anything, so
// every always_prove assertion of the paper's three examples other than
// that of f(1) is an error, and the always_diagnose one still warns.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

// The paper's always_prove label, and always_diagnose, "defined the same
// way".
struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics = {evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

namespace pre_example {
void f(int x) pre<always_prove>(x > 0);	// { dg-message "the predicate of the contract assertion" }

void g(int y)
{
  f(1);		// { dg-bogus "cannot be proven|always false" }
  f(y);		// { dg-error "contract assertion cannot be proven" }
  if (y > 0) {
    f(y);	// { dg-error "contract assertion cannot be proven" }
  }
}
}

namespace assert_example {
void f(int x)
{
  contract_assert<always_diagnose>(x > 0);	// { dg-warning "contract assertion cannot be proven" }
  if (x > 0) {
    contract_assert<always_prove>(x > 0);	// { dg-error "contract assertion cannot be proven" }
  }
}
}

namespace post_example {
int g(bool b)
  post<always_prove>(r : r > 0)	// { dg-error "contract assertion cannot be proven" }
{
  return b ? 17 : 34;
}
}
