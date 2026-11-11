// P4405 premises decided by this TU (T1-GCC): the allowed set.  When an
// assertion's label (P3400 allowed_semantics, which every TU sees in the
// declaration) admits only terminating semantics and assume, no TU can
// continue past a violation, so its evaluation is a guaranteed premise even
// where another TU's code may run: in a noinline replaceable callee the
// shadow calls, and in a callee inlined into the shadow but not into the real
// function (the parameters provoke that, as in p4405-premise-shadow-only.C)
// -- there only if the label also admits no semantic calling a handler, as
// settled_nothrow does: with settled, one TU's enforce handler may throw
// where another TU's quick_enforce copy traps, so the callee is a refinable
// body and the shadow may not rely on its code (M5-GCC).
// A label admitting observe, or none, settles nothing.  A caller-side
// evaluation is never settled (ignore is always admitted caller-side): a
// replaceable function holding one, here through its inlined wrapper, makes
// a shadow calling it out of line conditional.
//
// Each consumer is a diagnose evaluation: d_* can be proven only from the
// callee's assertion, i_* from a dominating test, so only what makes the
// whole shadow conditional matters there.  -fdump-tree-contract_shadow
// shows each result ("contract obligation N (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-allowed.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-allowed.json" }
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

// Only terminating semantics and assume: every TU's evaluation of an
// assertion with one of these labels guarantees its predicate.
constexpr allow<enforce, quick_enforce, noexcept_enforce, assume> settled{};
constexpr allow<enforce> enforce_only{};
// A continuing semantic is allowed: another TU may evaluate it so.
constexpr allow<enforce, observe> open{};

// Settled for the refinable-body mark too: no copy's evaluation calls a
// handler, so none may throw where another's does not.
constexpr allow<quick_enforce, noexcept_enforce, assume> settled_nothrow{};

int sink (int);
int big (int);

// label settled, always
[[gnu::always_inline]] inline void h_settled_always (int x) pre<settled> (x > 0) { sink (x); }

int d_settled_always (int x)
{
  h_settled_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_settled_always (int x, int y)
{
  h_settled_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// label settled, never
[[gnu::noinline]] inline void h_settled_never (int x) pre<settled> (x > 0) { sink (x); }

int d_settled_never (int x)
{
  h_settled_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_settled_never (int x, int y)
{
  h_settled_never (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// label enforce_only, always
[[gnu::always_inline]] inline void h_enforce_only_always (int x) pre<enforce_only> (x > 0) { sink (x); }

int d_enforce_only_always (int x)
{
  h_enforce_only_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_enforce_only_always (int x, int y)
{
  h_enforce_only_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// label enforce_only, never
[[gnu::noinline]] inline void h_enforce_only_never (int x) pre<enforce_only> (x > 0) { sink (x); }

int d_enforce_only_never (int x)
{
  h_enforce_only_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_enforce_only_never (int x, int y)
{
  h_enforce_only_never (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// label open, always
[[gnu::always_inline]] inline void h_open_always (int x) pre<open> (x > 0) { sink (x); }

int d_open_always (int x)
{
  h_open_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_open_always (int x, int y)
{
  h_open_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// label open, never
[[gnu::noinline]] inline void h_open_never (int x) pre<open> (x > 0) { sink (x); }

int d_open_never (int x)
{
  h_open_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_open_never (int x, int y)
{
  h_open_never (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

// label none, always
[[gnu::always_inline]] inline void h_none_always (int x) pre (x > 0) { sink (x); }

int d_none_always (int x)
{
  h_none_always (x);
  contract_assert (x > 0);	// sound
  return 0;
}

int i_none_always (int x, int y)
{
  h_none_always (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// label none, never
[[gnu::noinline]] inline void h_none_never (int x) pre (x > 0) { sink (x); }

int d_none_never (int x)
{
  h_none_never (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

int i_none_never (int x, int y)
{
  h_none_never (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

inline int h_provoked_settled (int x) pre<settled> (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g_provoked_settled (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = h_provoked_settled (y);
  contract_assert (y > 0);	// conditional
  return r + (y > 0 ? 1 : 0);
}

inline int h_provoked_open (int x) pre<open> (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g_provoked_open (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = h_provoked_open (y);
  contract_assert (y > 0);	// conditional
  return r + (y > 0 ? 1 : 0);
}

inline int h_provoked_settled_nothrow (int x) pre<settled_nothrow> (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g_provoked_settled_nothrow (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = h_provoked_settled_nothrow (y);
  contract_assert (y > 0);	// sound
  return r + (y > 0 ? 1 : 0);
}

// A caller-side evaluation is never settled.
namespace cs {
[[gnu::noinline]] void hc (int x) pre<settled> (x > 0) { sink (x); }
}
[[gnu::noinline]] inline void mid (int y) { cs::hc (y); }
[[gnu::noinline]] inline void mid2 (int y) { h_settled_never (y); }

int i_mid (int x, int y)
{
  mid (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

int i_mid2 (int x, int y)
{
  mid2 (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// So does one that inlined an unsettled callee-side evaluation; one
// that inlined a settled one does not.
[[gnu::noinline]] inline void mid3 (int y) { h_open_always (y); }
[[gnu::noinline]] inline void mid4 (int y) { h_settled_always (y); }

int i_mid3 (int x, int y)
{
  mid3 (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

int i_mid4 (int x, int y)
{
  mid4 (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

// { dg-final { scan-tree-dump {\(line 63\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 71\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 81\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 89\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 99\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 107\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 117\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 125\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 135\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 143\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 153\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 161\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 171\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 179\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 189\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 197\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 218\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 239\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 260\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 275\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 283\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 296\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 304\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {Calls void mid3\(int\)/[0-9]+ out of line} "contract_proven" } }
