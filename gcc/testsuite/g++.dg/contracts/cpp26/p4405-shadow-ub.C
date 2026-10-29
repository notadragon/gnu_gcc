// P4405 stage 3: in the proof shadow a path with undefined behavior is
// unreachable, exactly as an assumed predicate is, and relying on it is
// sound: language undefined behavior is a guaranteed premise.  After *p,
// p is not null; x + 1 does not overflow.  Proving that the
// undefined behavior does not happen is the business of an earlier
// assertion (P3100's implicit ones), not of the later one.
//
// Mirror: clang/test/Contracts/p4405-shadow-ub.cpp in the llvm_llvm-project
// fork; what each optimizer infers from undefined
// behavior differs.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -fdelete-null-pointer-checks" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fdump-tree-contract_shadow" }

int deref (int *p)
{
  int v = *p;
  contract_assert (p != nullptr);	// { dg-bogus "cannot be proven|always false" }
  return v;
}

int incremented (int x)
{
  int y = x + 1;
  contract_assert (y > x);		// { dg-bogus "cannot be proven|always false" }
  return y;
}

int unknown (int *p)
{
  contract_assert (p != nullptr);	// { dg-error "contract assertion cannot be proven" }
  return *p;
}

// { dg-final { scan-tree-dump {\(line 18\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 25\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 31\): unproven} "contract_shadow" } }
