// P4405 with P3400 labels: an always_prove label (allowed_semantics {prove},
// compute_semantic returning prove) and an always_diagnose label ({diagnose},
// computing diagnose), as D4405R0 defines them, work whatever semantic the
// build configures, and so does a label that only restricts the allowed set
// to {prove} or {diagnose}: adjusting any semantic to such a set reaches
// ignore's level, where diagnose and then prove are tried when ignore is not
// allowed (D3595R1, "Adjusting a Semantic to the Allowed Set").  Each
// namespace is configured with one semantic; the global namespace keeps the
// default, enforce.  An unprovable predicate shows which semantic applied.
// D4405: a precondition's analysis evaluation is at each call, so each
// function with one is called once, in ae2_calls, and diagnosed there.
//
// Mirror: clang/test/Contracts/p4405-labels-always.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-p4298 -fcontracts-allow-assume" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-labels-always.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic S>
struct always {
  using assertion_control_object = always;
  static constexpr evaluation_semantic_set allowed_semantics{S};
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return S; }
};
constexpr always<prove> always_prove{};
constexpr always<diagnose> always_diagnose{};

template<evaluation_semantic S>
struct only {
  using assertion_control_object = only;
  static constexpr evaluation_semantic_set allowed_semantics{S};
};
constexpr only<prove> prove_only{};
constexpr only<diagnose> diagnose_only{};

// The default, enforce.
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

namespace c_ignore {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_observe {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_enforce {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_quick_enforce {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_noexcept_observe {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_noexcept_enforce {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_assume {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_prove {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

namespace c_diagnose {
int ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int at () pre<always_prove> (true) { return 0; }	// { dg-bogus "cannot be proven|no valid" }
int ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int op (int x) pre<prove_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int od (int x) pre<diagnose_only> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
}

void
ae2_calls (int y)
{
  ap (y);	// { dg-error "contract assertion cannot be proven" }
  at ();	// { dg-bogus "cannot be proven|no valid" }
  ad (y);	// { dg-warning "contract assertion cannot be proven" }
  op (y);	// { dg-error "contract assertion cannot be proven" }
  od (y);	// { dg-warning "contract assertion cannot be proven" }
}
