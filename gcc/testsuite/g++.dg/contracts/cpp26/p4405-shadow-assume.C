// P4405 stage 3 with the P3100 assume semantic: an assume precondition or
// postcondition, now an optimizer assumption, reaches the proof shadow as a
// guaranteed assumption, so a proof relying on it is sound -- as an assume
// contract_assert's always did (p4405-shadow-previous.C).  The postcondition
// is assumed where its function is inlined.  The configuration selects
// assume by line; every other evaluation proves.
//
// Mirror: clang/test/Contracts/p4405-shadow-assume.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-assume.json" }

int sink (int);

int pre_assumed (int x)
  pre (x > 0)			// line 18: assume
{
  contract_assert (x > 0);	// line 20
  return sink (x);
}

static int post_assumed (int x)
  post (r: r > 0)		// line 25: assume
{
  return sink (x);
}

int use_post (int x)
{
  int r = post_assumed (x);
  contract_assert (r > 0);	// line 33
  return r;
}

// { dg-final { scan-tree-dump {\(line 20\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 33\): sound} "contract_shadow" } }
