// D4405 ("Taking the Address of a Function"): converting a closure to a
// pointer to function takes the address of its function call operator -- a
// call through the pointer, or through the static thunk it may point to,
// has no analysis evaluation.  So a precondition of the operator that
// allows no runtime semantic, and that folding does not prove, cannot be
// proven for a call through the address: an error under prove
// (always_prove) and a -Wcontract-unproven warning under diagnose
// (always_diagnose), for a lambda, a C++23 static lambda (whose conversion
// returns the operator itself) and a generic lambda (the specialization
// converted to).  A tautological one is not diagnosed.  The conversion
// functions themselves, defined with every capture-less lambda, and the
// thunk's call of the operator, whose arguments are unknown, are not
// analyzed.  Neither is an unevaluated conversion.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

auto unused = [] (int x) pre<always_prove> (x > 0) { return x; };	// { dg-bogus "address|cannot be proven" }
auto unused_static = [] (int x) static pre<always_prove> (x > 0) { return x; };	// { dg-bogus "address|cannot be proven" }

int
use (int y)
{
  auto l1 = [] (int x) pre<always_prove> (x > 0) { return x; };	// { dg-message "precondition here" }
  int (*p1) (int) = l1;					// { dg-error "contract assertion cannot be proven for a call through the address of 'use\\(int\\)::<lambda\\(int\\)>'" }
  auto l2 = [] (int x) static pre<always_prove> (x > 0) { return x; };	// { dg-message "precondition here" }
  int (*p2) (int) = l2;					// { dg-error "cannot be proven for a call through the address" }
  auto l3 = [] (auto x) pre<always_prove> (x > 0) { return x; };	// { dg-message "precondition here" }
  int (*p3) (int) = l3;					// { dg-error "cannot be proven for a call through the address" }
  auto l4 = [] (int x) pre<always_diagnose> (x > 0) { return x; };	// { dg-message "precondition here" }
  int (*p4) (int) = +l4;				// { dg-warning "contract assertion cannot be proven for a call through the address of 'use\\(int\\)::<lambda\\(int\\)>' \\\[-Wcontract-unproven\\\]" }
  auto p5 = static_cast<int (*) (int)> (l4);		// { dg-warning "cannot be proven for a call through the address" }
  auto l6 = [] (auto x) static pre<always_diagnose> (x > 0) { return x; };	// { dg-message "precondition here" }
  long (*p6) (long) = l6;				// { dg-warning "cannot be proven for a call through the address" }
  auto l7 = [] (int x) pre (x > 0) { return x; };
  int (*p7) (int) = l7;					// { dg-bogus "address|analyze" }
  auto l8 = [] (int x) pre<always_prove> (sizeof (x) == sizeof (int)) { return x; };	// { dg-bogus "precondition here" }
  int (*p8) (int) = l8;					// { dg-bogus "address" }
  decltype (+l1) q = nullptr;				// { dg-bogus "address" }
  (void) sizeof (+l2);					// { dg-bogus "address" }
  int r = l1 (1) + l2 (2) + l3 (3) + l4 (4) + l6 (6);	// { dg-bogus "cannot be proven|address" }
  return r + p1 (y) + p2 (y) + p3 (y) + p4 (y) + p5 (y) + p6 (y) + p7 (y) + p8 (y) + (q ? 1 : 0);
}

// In a template, at instantiation.
template<typename T>
T
in_template (T y)
{
  auto l = [] (T x) pre<always_prove> (x > 0) { return x; };	// { dg-message "precondition here" }
  T (*p) (T) = l;					// { dg-error "cannot be proven for a call through the address" }
  return p (y);
}

template int in_template<int> (int);
