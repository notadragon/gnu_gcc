// P4405 premises decided by this TU (P6-GCC, mirroring T2-Clang): what the
// shadow relies on through more than one function.  Every assertion but the
// consumers is enforce.
// * through_exact: the shadow calls out of line x_calls, an external
//   function, which calls out of line the replaceable h_never holding a
//   premise.  IPA summaries of h_never (noreturn, const/pure, mod-ref,
//   return ranges) can reach the shadow through x_calls, so it is
//   conditional, as for a direct call (out_of_line_premises_p is
//   transitive through a non-replaceable intermediate).
// * through_inlining: the shadow calls out of line x_inlines, an external
//   function whose REAL body inlined the replaceable h_auto.  x_inlines is
//   this TU's code and so is what it inlined: sound.
// * chain: the shadow inlines mid, a replaceable function holding no
//   premise of its own, and through it h_chain, which holds one.  The
//   shadow records the chain of call sites; the real chain inlines both, so
//   sound.  (Clang forces the inlinings; GCC checks the real function made
//   them.)
// * static_chain: the same through mid_static, a static function: sound.
// The optimized scans confirm the inlining each outcome rests on.
//
// Each consumer is a diagnose evaluation: d_* can be proven only from the
// callee's assertion, i_* from a dominating test, so only what makes the
// whole shadow conditional matters there.  -fdump-tree-contract_shadow
// shows each result ("contract obligation N (line L): sound").
//
// Mirror: clang/test/Contracts/p4405-premise-transitive.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-transitive.json" }

int sink (int);

[[gnu::noinline]] inline void h_never (int x) pre (x > 0) { sink (x); }
[[gnu::noinline]] void x_calls (int y) { h_never (y); }

int through_exact (int x, int y)
{
  x_calls (y);
  if (x > 0)
    contract_assert (x > 0);	// conditional
  return 0;
}

inline int h_auto (int x) pre (x > 0) { return sink (x); }
[[gnu::noinline]] int x_inlines (int y) { return h_auto (y) + 1; }

int through_inlining (int x, int y)
{
  x_inlines (y);
  if (x > 0)
    contract_assert (x > 0);	// sound
  return 0;
}

inline int h_chain (int x) pre (x > 0) { return sink (x); }
inline int mid (int x) { return h_chain (x) + sink (x + 1); }

int chain (int y)
{
  int r = mid (y);
  contract_assert (y > 0);	// sound
  return r;
}

inline int h_chain2 (int x) pre (x > 0) { return sink (x); }
static int mid_static (int x) { return h_chain2 (x) + sink (x + 1); }

int static_chain (int y)
{
  int r = mid_static (y);
  contract_assert (y > 0);	// sound
  return r;
}

// { dg-final { scan-tree-dump {\(line 41\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 52\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 62\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 72\): sound} "contract_shadow" } }
// through_exact: both calls stay out of line.
// { dg-final { scan-tree-dump {x_calls \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {h_never \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump {Calls void x_calls\(int\)/[0-9]+ out of line} "contract_proven" } }
// x_inlines, chain and static_chain inlined their callees.
// { dg-final { scan-tree-dump {x_inlines \(y_[0-9]+\(D\)\)} "optimized" } }
// { dg-final { scan-tree-dump-not {(h_auto|mid|h_chain|mid_static|h_chain2) \(} "optimized" } }
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: assumed} 3 "contract_proven" } }
