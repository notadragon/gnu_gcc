// P4405 premises decided by this TU (T1-GCC): a replaceable function
// given an assumption by -fcontracts-prove-assume exports nothing its late
// optimizations learn from it.  In f and fs an earlier quick_enforce lets
// the shadow prove the diagnose evaluation soundly (an own-body premise),
// and the assumption made of it tells the late value-range pass that the
// result is positive.  For the static fs that range is used by use_fs,
// compiled later, which drops its test of the result; f is inline and holds
// contract evaluations, a refinable body: the definition the program calls
// may be another TU's, which assumes nothing, so use_f uses nothing f's
// late passes find (its result's range, pure, const, nothrow, noreturn) and
// keeps its test.  p4405-premise-late-summary-caller.C has a function whose
// only contract evaluation is a caller-side one.
//
// Mirror: clang/test/Contracts/p4405-premise-late-summary.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-contract_proven -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-late-summary.json" }

int bad ();

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);	// quick_enforce
  contract_assert (x > 0);	// sound
  return x;
}

int use_f (int y)
{
  int r = f (y);
  if (r <= 0)
    return bad ();
  return r;
}

[[gnu::noinline]] static int fs (int x)
{
  contract_assert (x > 0);	// quick_enforce
  contract_assert (x > 0);	// sound
  return x;
}

int use_fs (int y)
{
  int r = fs (y);
  if (r <= 0)
    return bad ();
  return r;
}

// { dg-final { scan-tree-dump {\(line 25\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 40\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {contract obligation [0-9]+: assumed} 2 "contract_proven" } }
// Only use_f still calls bad.
// { dg-final { scan-tree-dump-times {bad \(\)} 1 "optimized" } }
