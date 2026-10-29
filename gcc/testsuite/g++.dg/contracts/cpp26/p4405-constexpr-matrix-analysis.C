// D4405 ("The Analysis Evaluation", DECISIONS.md E142): constant evaluation
// under each label, configured -fcontract-analysis-semantic=prove and
// -fcontract-evaluation-semantic=ignore (an analysis configuration never
// applies to constant evaluation).  Constant evaluation has no analysis
// evaluation: the semantic it resolves to is adjusted to the whole allowed
// set, and a prove is then evaluated as enforce (a violation is an error,
// and the expression is not constant) and a diagnose as observe (a warning,
// and the evaluation continues) -- never as ignore, which would let the
// false predicate pass silently.  An analysis_semantic facet only
// strengthens the analysis evaluation, so constant evaluation does not see
// it; a compute_semantic result of prove or diagnose does count.  A
// postcondition and a contract_assert also have their analysis evaluations
// in the body, outside constant evaluation.  A precondition's is at each
// call, but every call here is in a constexpr variable's initializer, which
// is manifestly constant-evaluated, so none is analyzed (D4405R0: "During
// constant evaluation there is no analysis evaluation").
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-additional-options "-fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace L = std::contracts::labels;
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
constexpr analyzed<prove> proven{};

template<evaluation_semantic S>
struct computes {
  using assertion_control_object = computes;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return S; }
};
constexpr computes<prove> computes_prove{};
constexpr computes<diagnose> computes_diagnose{};


// no label: analysis prove; constant evaluation ignore.
constexpr int pre_none (int x) pre (x > 0) { return x; }	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_none (int x) post (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
constexpr int assert_none (int x) { contract_assert (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
constexpr int pre_none_ok = pre_none (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_none_bad = pre_none (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (pre_none_bad == -1);
constexpr int post_none_ok = post_none (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_none_bad = post_none (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (post_none_bad == -1);
constexpr int assert_none_ok = assert_none (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_none_bad = assert_none (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (assert_none_bad == -1);

// always_prove: analysis prove; constant evaluation enforce.
constexpr int pre_ap (int x) pre<always_prove> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int post_ap (int x) post<always_prove> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-error "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int assert_ap (int x) { contract_assert<always_prove> (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-error "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int pre_ap_ok = pre_ap (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_ap_bad = pre_ap (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int post_ap_ok = post_ap (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_ap_bad = post_ap (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int assert_ap_ok = assert_ap (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_ap_bad = assert_ap (-1);	// { dg-bogus "always false|cannot be proven" }

// always_diagnose: analysis diagnose; constant evaluation observe.
constexpr int pre_ad (int x) pre<always_diagnose> (x > 0) { return x; }	// { dg-warning "contract predicate is false in constant expression" }
constexpr int post_ad (int x) post<always_diagnose> (r: r > 0) { return x; }	// { dg-warning "contract assertion cannot be proven" }
// { dg-warning "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int assert_ad (int x) { contract_assert<always_diagnose> (x > 0); return x; }	// { dg-warning "contract assertion cannot be proven" }
// { dg-warning "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int pre_ad_ok = pre_ad (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_ad_bad = pre_ad (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (pre_ad_bad == -1);
constexpr int post_ad_ok = post_ad (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_ad_bad = post_ad (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (post_ad_bad == -1);
constexpr int assert_ad_ok = assert_ad (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_ad_bad = assert_ad (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (assert_ad_bad == -1);

// L::terminating: analysis prove; constant evaluation enforce.
constexpr int pre_term (int x) pre<L::terminating> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int post_term (int x) post<L::terminating> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-error "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int assert_term (int x) { contract_assert<L::terminating> (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-error "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int pre_term_ok = pre_term (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_term_bad = pre_term (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int post_term_ok = post_term (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_term_bad = post_term (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int assert_term_ok = assert_term (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_term_bad = assert_term (-1);	// { dg-bogus "always false|cannot be proven" }

// L::symbolic: analysis prove; constant evaluation ignore.
constexpr int pre_sym (int x) pre<L::symbolic> (x > 0) { return x; }	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_sym (int x) post<L::symbolic> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
constexpr int assert_sym (int x) { contract_assert<L::symbolic> (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
constexpr int pre_sym_ok = pre_sym (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_sym_bad = pre_sym (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (pre_sym_bad == -1);
constexpr int post_sym_ok = post_sym (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_sym_bad = post_sym (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (post_sym_bad == -1);
constexpr int assert_sym_ok = assert_sym (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_sym_bad = assert_sym (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (assert_sym_bad == -1);

// L::always_enforce: analysis ignore; constant evaluation enforce.
constexpr int pre_ae (int x) pre<L::always_enforce> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int post_ae (int x) post<L::always_enforce> (r: r > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int assert_ae (int x) { contract_assert<L::always_enforce> (x > 0); return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int pre_ae_ok = pre_ae (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_ae_bad = pre_ae (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int post_ae_ok = post_ae (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_ae_bad = post_ae (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int assert_ae_ok = assert_ae (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_ae_bad = assert_ae (-1);	// { dg-bogus "always false|cannot be proven" }

// proven: analysis prove; constant evaluation ignore.
constexpr int pre_fp (int x) pre<proven> (x > 0) { return x; }	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_fp (int x) post<proven> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
constexpr int assert_fp (int x) { contract_assert<proven> (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
constexpr int pre_fp_ok = pre_fp (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_fp_bad = pre_fp (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (pre_fp_bad == -1);
constexpr int post_fp_ok = post_fp (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_fp_bad = post_fp (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (post_fp_bad == -1);
constexpr int assert_fp_ok = assert_fp (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_fp_bad = assert_fp (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (assert_fp_bad == -1);

// computes_prove: analysis prove; constant evaluation enforce.
constexpr int pre_cp (int x) pre<computes_prove> (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int post_cp (int x) post<computes_prove> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-error "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int assert_cp (int x) { contract_assert<computes_prove> (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-error "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int pre_cp_ok = pre_cp (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_cp_bad = pre_cp (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int post_cp_ok = post_cp (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_cp_bad = post_cp (-1);	// { dg-bogus "always false|cannot be proven" }
constexpr int assert_cp_ok = assert_cp (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_cp_bad = assert_cp (-1);	// { dg-bogus "always false|cannot be proven" }

// computes_diagnose: analysis prove; constant evaluation observe.
constexpr int pre_cd (int x) pre<computes_diagnose> (x > 0) { return x; }	// { dg-warning "contract predicate is false in constant expression" }
constexpr int post_cd (int x) post<computes_diagnose> (r: r > 0) { return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-warning "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int assert_cd (int x) { contract_assert<computes_diagnose> (x > 0); return x; }	// { dg-error "contract assertion cannot be proven" }
// { dg-warning "contract predicate is false in constant expression" "" { target *-*-* } .-1 }
constexpr int pre_cd_ok = pre_cd (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int pre_cd_bad = pre_cd (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (pre_cd_bad == -1);
constexpr int post_cd_ok = post_cd (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int post_cd_bad = post_cd (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (post_cd_bad == -1);
constexpr int assert_cd_ok = assert_cd (1);	// { dg-bogus "false in constant expression|always false|cannot be proven" }
constexpr int assert_cd_bad = assert_cd (-1);	// { dg-bogus "always false|cannot be proven" }
static_assert (assert_cd_bad == -1);

