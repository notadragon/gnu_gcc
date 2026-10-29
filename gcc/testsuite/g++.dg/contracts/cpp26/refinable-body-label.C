// The refinable-body rule with P3400 labels (contract_evaluation_settled_p):
// an evaluation is settled, and does not make its function a refinable
// body, when the label's allowed_semantics facet -- which every translation
// unit sees in the declaration -- admits only semantics that do not continue
// past a violation (direction A: facts from one definition hold for every
// other) and none that lets an exception leave the function but some do not
// (direction B): a subset of {quick_enforce, noexcept_enforce, assume}, or
// {enforce} alone, or in a noexcept function any subset of {enforce,
// quick_enforce, noexcept_enforce, assume}.  (refinable-fork-label.C runs
// the sets with noexcept_enforce and assume.)  Callers of an inline function
// whose evaluations are all settled use its facts; callers of one holding
// an evaluation that is not settled, or none with a label, do not.
//
// a_*: return value range (the caller's comparison folds or stays).
// use_b_*: nothrow (the caller's landing pad goes or stays).
// h_*: a P3097 wrapper for a virtual call, which evaluates the callee's
// postcondition with its callee-side semantic: settled or not as for an
// assertion in the body.
// c_settled: a caller-side evaluation (refinable-body-label.json) is never
// settled, whatever the label, because ignore is always allowed caller-side;
// its function is marked, but the callee is not (use_d folds).
// refinable-body-p3097.C has the C-variadic and destructor P3097 callers.
//
// Mirror: clang/test/Contracts/refinable-body-label.cpp (the nothrow half)
// and refinable-body-p3097.cpp (h_settled and h_open: Clang marks both).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p3097" }
// { dg-additional-options "-fcontract-evaluation-semantic=quick_enforce -O2 -fno-ipa-sra -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/refinable-body-label.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

inline constexpr allow<quick_enforce> l_quick{};
inline constexpr allow<enforce> l_enforce{};
inline constexpr allow<enforce, quick_enforce> l_mixed{};
inline constexpr allow<enforce, observe> l_open{};

int g (int);

// Settled.
[[gnu::noinline]] inline int a_settled (int x) { contract_assert<l_quick> (x > 0); return x; }
[[gnu::noinline]] inline int a_enforce (int x) { contract_assert<l_enforce> (x > 0); return x; }
[[gnu::noinline]] inline int a_mixed_nx (int x) noexcept { contract_assert<l_mixed> (x > 0); return x; }
[[gnu::noinline]] inline int a_pre (int x) pre<l_quick> (x > 0) { return x; }

// Not settled.
[[gnu::noinline]] inline int a_mixed (int x) { contract_assert<l_mixed> (x > 0); return x; }
[[gnu::noinline]] inline int a_open (int x) { contract_assert<l_open> (x > 0); return x; }
[[gnu::noinline]] inline int a_none (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] inline int a_pre_none (int x) pre (x > 0) { return x; }
[[gnu::noinline]] inline int a_pre_some (int x) pre<l_quick> (x > 0) pre (x > 1) { return x; }

int use_a_settled (int y) { return a_settled (y) > 0 ? 7000101 : 7000201; }
int use_a_enforce (int y) { return a_enforce (y) > 0 ? 7000102 : 7000202; }
int use_a_mixed_nx (int y) { return a_mixed_nx (y) > 0 ? 7000103 : 7000203; }
int use_a_pre (int y) { return a_pre (y) > 0 ? 7000104 : 7000204; }
int use_a_mixed (int y) { return a_mixed (y) > 0 ? 7000111 : 7000211; }
int use_a_open (int y) { return a_open (y) > 0 ? 7000112 : 7000212; }
int use_a_none (int y) { return a_none (y) > 0 ? 7000113 : 7000213; }
int use_a_pre_none (int y) { return a_pre_none (y) > 0 ? 7000114 : 7000214; }
int use_a_pre_some (int y) { return a_pre_some (y) > 0 ? 7000115 : 7000215; }

int use_b_settled (int y) { try { return a_settled (y); } catch (...) { return g (1); } }
int use_b_mixed (int y) { try { return a_mixed (y); } catch (...) { return g (2); } }

struct B
{
  virtual int vs (int x) post<l_quick> (r: r > 0);
  virtual int vo (int x) post (r: r > 0);
};

[[gnu::noinline]] inline int h_settled (B *p, int x) { return p->vs (x); }
[[gnu::noinline]] inline int h_open (B *p, int x) { return p->vo (x); }

int use_h_settled (B *p, int y) { return h_settled (p, y) > 0 ? 7000105 : 7000205; }
int use_h_open (B *p, int y) { return h_open (p, y) > 0 ? 7000116 : 7000216; }

[[gnu::noinline]] inline int d (int x) post<l_quick> (r: r > 0) { return x; }

[[gnu::noinline]] inline int
c_settled (int x)
{
  return d (x);		// caller-side quick_enforce
}

int use_c (int y) { return c_settled (y) > 0 ? 7000117 : 7000217; }
int use_d (int y) { return d (y) > 0 ? 7000106 : 7000206; }

// { dg-final { scan-tree-dump-not "\\m700020\[1-6\]\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000211\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000212\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000213\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000214\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000215\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000216\\M" "optimized" } }
// { dg-final { scan-tree-dump "\\m7000217\\M" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(1\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(2\\)" "optimized" } }
