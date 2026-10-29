// P4405 premises decided by this TU (T1-GCC): a callee inlined into the
// shadow but not into the real function.  The parameters make the IPA
// inliner do that: each g_* has a large branch that an earlier assume
// evaluation makes dead in the shadow only (its predicate is not simple, so
// the real function's assumption is outlined, and unused before the late
// passes), so the real function exceeds large-function-growth and the
// shadow does not.  An inline or template callee's enforce
// precondition is then not decided here -- the real function calls the
// callee, and the definition linked may evaluate it with ignore -- so the
// proof is conditional and nothing is assumed (before the rule it was sound,
// and -fcontracts-prove-assume folded `y > 0' to true in the real function).
// A static callee's copy is this TU's: sound, assumed.  Observe leaves no
// premise.  p4405-premise-allowed.C has the label that settles the inline
// case.
//
// Each consumer is a diagnose evaluation: d_* can be proven only from the
// callee's assertion, i_* from a dominating test, so only what makes the
// whole shadow conditional matters there.  -fdump-tree-contract_shadow
// shows each result ("contract obligation N (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-shadow-only.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "--param large-function-insns=40 --param large-function-growth=10" }
// { dg-additional-options "--param max-inline-insns-single=200 --param max-inline-insns-auto=200 -fno-partial-inlining" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-shadow-only.json" }

int sink (int);
int big (int);

namespace sem_enforce_inline {
inline int h_inline (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_inline (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = sem_enforce_inline::h_inline (y);
  contract_assert (y > 0);	// conditional
  return r + (y > 0 ? 1 : 0);
}

namespace sem_enforce_static {
static int h_static (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_static (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = sem_enforce_static::h_static (y);
  contract_assert (y > 0);	// sound
  return r + (y > 0 ? 1 : 0);
}

namespace sem_enforce_template {
template <class T>
int h_template (T x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_template (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = sem_enforce_template::h_template<int> (y);
  contract_assert (y > 0);	// conditional
  return r + (y > 0 ? 1 : 0);
}

namespace sem_assume_assume {
inline int h_assume (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_assume (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = sem_assume_assume::h_assume (y);
  contract_assert (y > 0);	// conditional
  return r + (y > 0 ? 1 : 0);
}

namespace sem_observe_observe {
inline int h_observe (int x) pre (x > 0) {
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}
}

int g_observe (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = sem_observe_observe::h_observe (y);
  contract_assert (y > 0);	// { dg-warning "contract assertion cannot be proven" }
  return r + (y > 0 ? 1 : 0);
}

// { dg-final { scan-tree-dump {\(line 51\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 74\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 98\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 121\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 144\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {= sem_enforce_inline::h_inline \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {= sem_enforce_static::h_static \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {= sem_enforce_template::h_template<int> \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {= sem_assume_assume::h_assume \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {= sem_observe_observe::h_observe \(y_[0-9]+\(D\)\)} "optimized" } }
// Assumed only for the static callee: `y > 0 ? 1 : 0' folded to 1
// there, kept in the other provoked callers.
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: assumed} 1 "contract_proven" } }
// { dg-final { scan-tree-dump-times {y_[0-9]+\(D\) > 0} 4 "optimized" } }
