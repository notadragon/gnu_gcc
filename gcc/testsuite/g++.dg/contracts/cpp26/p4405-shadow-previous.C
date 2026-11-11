// P4405 stage 3: the proof shadow assumes earlier evaluations.  A proof
// relying on one with a terminating semantic (enforce, quick_enforce) or
// assume is sound; one relying on ignore or observe is conditional.  Both
// suppress the diagnostic, so the -fdump-tree-contract_shadow dump tells
// them apart.  An unrelated earlier observe still makes every proof in
// that shadow conditional (an assumption can affect what precedes it).
// The configuration selects each earlier evaluation's semantic by its
// line; every other evaluation proves.
//
// Mirror: clang/test/Contracts/p4405-shadow-previous.cpp in the
// llvm_llvm-project fork, with its own way of showing the result.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-previous.json" }

int sink (int);

int enforced (int x)
{
  contract_assert (x > 0);	// line 21: enforce
  contract_assert (x > 0);	// line 22
  return sink (x);
}

int quick (int x)
{
  contract_assert (x > 0);	// line 28: quick_enforce
  contract_assert (x > 0);	// line 29
  return sink (x);
}

int assumed (int x)
{
  contract_assert (x > 0);	// line 35: assume
  contract_assert (x > 0);	// line 36
  return sink (x);
}

int observed (int x)
{
  contract_assert (x > 0);	// line 42: observe
  contract_assert (x > 0);	// line 43
  return sink (x);
}

int ignored (int x)
{
  contract_assert (x > 0);	// line 49: ignore
  contract_assert (x > 0);	// line 50
  return sink (x);
}

int unrelated (int x, int y)
{
  if (x > 0)
    {
      contract_assert (x > 0);	// line 58
      sink (x);
    }
  contract_assert (y > 0);	// line 61: observe
  return 0;
}

// { dg-final { scan-tree-dump {\(line 22\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 29\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 36\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 43\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 50\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 58\): conditional} "contract_shadow" } }
