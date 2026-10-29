// D4405 ("Adjusting to the Allowed Semantics", "P3400R5 Analysis
// Evaluation Configuration"): the label x configuration matrix, configured by flags.
// Configured: analysis ignore, runtime ignore.  Each label's assertions are
// a precondition (analyzed at each call: unguarded, guarded, and with a
// constant argument), an unprovable and a provable postcondition, and an
// unguarded and a guarded contract_assert.  The analysis semantic follows
// the label: none, symbolic and an analysis_semantic facet take the
// configured one (a facet the stronger of the two), always_prove and
// always_diagnose force theirs, terminating keeps only prove, always_enforce
// is never analyzed, a combined label takes the stronger facet adjusted to
// the combined allowed set, and a compute_semantic result of prove or
// diagnose moves to the analysis evaluation.  The runtime configuration
// never changes what is diagnosed.  One file of the matrix
// (p4405-analysis-matrix-*.C); optimized, so the proof shadow proves the
// guarded cases.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-analysis-semantic=ignore -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace L = std::contracts::labels;
using L::operator|;
using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set allowed_semantics{prove};
};
constexpr always_prove_t always_prove{};
struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set allowed_semantics{diagnose};
};
constexpr always_diagnose_t always_diagnose{};

template<evaluation_semantic S>
struct analyzed {
  using assertion_control_object = analyzed;
  constexpr evaluation_semantic analysis_semantic () const { return S; }
};
constexpr analyzed<ignore> unanalyzed{};
constexpr analyzed<diagnose> diagnosed{};
constexpr analyzed<prove> proven{};

template<evaluation_semantic S>
struct computes {
  using assertion_control_object = computes;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return S; }
};
constexpr computes<prove> computes_prove{};
constexpr computes<diagnose> computes_diagnose{};

// no label: analysis ignore.
int pre_none (int x) pre (x > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int post_none (int x) post (r: r > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int postok_none (int x) post (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_none (int x)
{
  contract_assert (x > 0);	// { dg-bogus "cannot be proven|always false" }
  if (x > 0)
    contract_assert (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_none (int y)
{
  pre_none (y);	// { dg-bogus "cannot be proven|always false" }
  if (y > 0)
    pre_none (y);	// { dg-bogus "cannot be proven|always false" }
  pre_none (1);	// { dg-bogus "cannot be proven|always false" }
}

// always_prove: analysis prove.
int pre_ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_ap (int x) post<always_prove> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
int postok_ap (int x) post<always_prove> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_ap (int x)
{
  contract_assert<always_prove> (x > 0);	// { dg-error "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert<always_prove> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_ap (int y)
{
  pre_ap (y);	// { dg-error "contract assertion cannot be proven" }
  if (y > 0)
    pre_ap (y);	// { dg-bogus "cannot be proven|always false" }
  pre_ap (1);	// { dg-bogus "cannot be proven|always false" }
}

// always_diagnose: analysis diagnose.
int pre_ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_ad (int x) post<always_diagnose> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
int postok_ad (int x) post<always_diagnose> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_ad (int x)
{
  contract_assert<always_diagnose> (x > 0);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (x > 0)
    contract_assert<always_diagnose> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_ad (int y)
{
  pre_ad (y);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (y > 0)
    pre_ad (y);	// { dg-bogus "cannot be proven|always false" }
  pre_ad (1);	// { dg-bogus "cannot be proven|always false" }
}

// L::terminating: analysis ignore.
int pre_term (int x) pre<L::terminating> (x > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int post_term (int x) post<L::terminating> (r: r > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int postok_term (int x) post<L::terminating> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_term (int x)
{
  contract_assert<L::terminating> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  if (x > 0)
    contract_assert<L::terminating> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_term (int y)
{
  pre_term (y);	// { dg-bogus "cannot be proven|always false" }
  if (y > 0)
    pre_term (y);	// { dg-bogus "cannot be proven|always false" }
  pre_term (1);	// { dg-bogus "cannot be proven|always false" }
}

// L::symbolic: analysis ignore.
int pre_sym (int x) pre<L::symbolic> (x > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int post_sym (int x) post<L::symbolic> (r: r > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int postok_sym (int x) post<L::symbolic> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_sym (int x)
{
  contract_assert<L::symbolic> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  if (x > 0)
    contract_assert<L::symbolic> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_sym (int y)
{
  pre_sym (y);	// { dg-bogus "cannot be proven|always false" }
  if (y > 0)
    pre_sym (y);	// { dg-bogus "cannot be proven|always false" }
  pre_sym (1);	// { dg-bogus "cannot be proven|always false" }
}

// L::always_enforce: analysis ignore.
int pre_ae (int x) pre<L::always_enforce> (x > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int post_ae (int x) post<L::always_enforce> (r: r > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int postok_ae (int x) post<L::always_enforce> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_ae (int x)
{
  contract_assert<L::always_enforce> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  if (x > 0)
    contract_assert<L::always_enforce> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_ae (int y)
{
  pre_ae (y);	// { dg-bogus "cannot be proven|always false" }
  if (y > 0)
    pre_ae (y);	// { dg-bogus "cannot be proven|always false" }
  pre_ae (1);	// { dg-bogus "cannot be proven|always false" }
}

// diagnosed: analysis diagnose.
int pre_fd (int x) pre<diagnosed> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_fd (int x) post<diagnosed> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
int postok_fd (int x) post<diagnosed> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fd (int x)
{
  contract_assert<diagnosed> (x > 0);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (x > 0)
    contract_assert<diagnosed> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fd (int y)
{
  pre_fd (y);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (y > 0)
    pre_fd (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fd (1);	// { dg-bogus "cannot be proven|always false" }
}

// proven: analysis prove.
int pre_fp (int x) pre<proven> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_fp (int x) post<proven> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
int postok_fp (int x) post<proven> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fp (int x)
{
  contract_assert<proven> (x > 0);	// { dg-error "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert<proven> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fp (int y)
{
  pre_fp (y);	// { dg-error "contract assertion cannot be proven" }
  if (y > 0)
    pre_fp (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fp (1);	// { dg-bogus "cannot be proven|always false" }
}

// diagnosed | proven: analysis prove.
int pre_fdfp (int x) pre<diagnosed | proven> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_fdfp (int x) post<diagnosed | proven> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
int postok_fdfp (int x) post<diagnosed | proven> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fdfp (int x)
{
  contract_assert<diagnosed | proven> (x > 0);	// { dg-error "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert<diagnosed | proven> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fdfp (int y)
{
  pre_fdfp (y);	// { dg-error "contract assertion cannot be proven" }
  if (y > 0)
    pre_fdfp (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fdfp (1);	// { dg-bogus "cannot be proven|always false" }
}

// unanalyzed | diagnosed: analysis diagnose.
int pre_fudi (int x) pre<unanalyzed | diagnosed> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_fudi (int x) post<unanalyzed | diagnosed> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
int postok_fudi (int x) post<unanalyzed | diagnosed> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fudi (int x)
{
  contract_assert<unanalyzed | diagnosed> (x > 0);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (x > 0)
    contract_assert<unanalyzed | diagnosed> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fudi (int y)
{
  pre_fudi (y);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (y > 0)
    pre_fudi (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fudi (1);	// { dg-bogus "cannot be proven|always false" }
}

// diagnosed | L::symbolic: analysis diagnose.
int pre_fdsym (int x) pre<diagnosed | L::symbolic> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_fdsym (int x) post<diagnosed | L::symbolic> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
int postok_fdsym (int x) post<diagnosed | L::symbolic> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fdsym (int x)
{
  contract_assert<diagnosed | L::symbolic> (x > 0);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (x > 0)
    contract_assert<diagnosed | L::symbolic> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fdsym (int y)
{
  pre_fdsym (y);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (y > 0)
    pre_fdsym (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fdsym (1);	// { dg-bogus "cannot be proven|always false" }
}

// diagnosed | L::terminating: analysis ignore.
int pre_fdterm (int x) pre<diagnosed | L::terminating> (x > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int post_fdterm (int x) post<diagnosed | L::terminating> (r: r > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int postok_fdterm (int x) post<diagnosed | L::terminating> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fdterm (int x)
{
  contract_assert<diagnosed | L::terminating> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  if (x > 0)
    contract_assert<diagnosed | L::terminating> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fdterm (int y)
{
  pre_fdterm (y);	// { dg-bogus "cannot be proven|always false" }
  if (y > 0)
    pre_fdterm (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fdterm (1);	// { dg-bogus "cannot be proven|always false" }
}

// proven | L::always_enforce: analysis ignore.
int pre_fpae (int x) pre<proven | L::always_enforce> (x > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int post_fpae (int x) post<proven | L::always_enforce> (r: r > 0) { return x; }	// { dg-bogus "cannot be proven|always false" }
int postok_fpae (int x) post<proven | L::always_enforce> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_fpae (int x)
{
  contract_assert<proven | L::always_enforce> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  if (x > 0)
    contract_assert<proven | L::always_enforce> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_fpae (int y)
{
  pre_fpae (y);	// { dg-bogus "cannot be proven|always false" }
  if (y > 0)
    pre_fpae (y);	// { dg-bogus "cannot be proven|always false" }
  pre_fpae (1);	// { dg-bogus "cannot be proven|always false" }
}

// computes_prove: analysis prove.
int pre_cp (int x) pre<computes_prove> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_cp (int x) post<computes_prove> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
int postok_cp (int x) post<computes_prove> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_cp (int x)
{
  contract_assert<computes_prove> (x > 0);	// { dg-error "contract assertion cannot be proven" }
  if (x > 0)
    contract_assert<computes_prove> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_cp (int y)
{
  pre_cp (y);	// { dg-error "contract assertion cannot be proven" }
  if (y > 0)
    pre_cp (y);	// { dg-bogus "cannot be proven|always false" }
  pre_cp (1);	// { dg-bogus "cannot be proven|always false" }
}

// computes_diagnose: analysis diagnose.
int pre_cd (int x) pre<computes_diagnose> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int post_cd (int x) post<computes_diagnose> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
int postok_cd (int x) post<computes_diagnose> (r: r > 0) { return x > 0 ? x : 1; }	// { dg-bogus "cannot be proven|always false" }
int assert_cd (int x)
{
  contract_assert<computes_diagnose> (x > 0);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (x > 0)
    contract_assert<computes_diagnose> (x > 0);	// { dg-bogus "cannot be proven|always false" }
  return x;
}
void calls_cd (int y)
{
  pre_cd (y);	// { dg-warning "contract assertion cannot be proven \\\[-Wcontract-unproven\\\]" }
  if (y > 0)
    pre_cd (y);	// { dg-bogus "cannot be proven|always false" }
  pre_cd (1);	// { dg-bogus "cannot be proven|always false" }
}
