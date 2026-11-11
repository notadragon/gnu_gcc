// P4405, DECISIONS.md E169: an analysis evaluation depends on the analysis
// evaluations of the preconditions of the calls in its predicate, though
// the predicate is never evaluated at run time (runtime ignore here).  A
// nested prove precondition that cannot be proven, or is always false,
// makes the outer assertion unproven: "contract assertion cannot be
// proven" at the outer, with a note at the nested precondition, which is
// not reported on its own.  At -O2 the proof shadow decides both, the
// nested one where its call is evaluated in the outer's region: proven
// after a guard, and caught before the undefined behaviour in the callee
// (`10 / 0') could make the rest of the path unreachable, whether folding
// decides the nested one (`g3 (0)') or the shadow does (`g5 (y)' after
// `y != 0' returned).  The same at -O0,
// folding alone: p4405-nested-prove-O0.C.
//
// Mirror: clang/test/Contracts/p4405-nested-prove.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

int g1 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g1 (int) { return 1; }
int g2 (int x) pre (x != 0);	// { dg-bogus "precondition of a call" }
int g2 (int) { return 1; }
int g3 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate is always false" }
int g3 (int x) { return 10 / x; }
int g4 (int x) pre (x != 0);	// { dg-bogus "precondition of a call" }
int g4 (int) { return 1; }
int g5 (int x) pre (x != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
int g5 (int x) { return 10 / x; }

int
unprovable (int y)
{
  contract_assert (g1 (y) == 1);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
guarded (int y)
{
  if (y == 0)
    return 0;
  contract_assert (g2 (y) == 1);	// { dg-bogus "cannot be proven" }
  return y;
}

int
always_false (int y)
{
  contract_assert (g3 (0) != 7);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
guarded_zero (int y)
{
  if (y != 0)
    return 0;
  contract_assert (g5 (y) != 7);	// { dg-error "contract assertion cannot be proven" }
  return y;
}

int
provable (int y)
{
  contract_assert (g4 (5) == 1);	// { dg-bogus "cannot be proven" }
  return y;
}

// { dg-final { scan-tree-dump {\(line 37\): nested in} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 37\), with those nested in it: unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 46\), with those nested in it: sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 53\), with those nested in it: unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 53\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 62\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 62\), with those nested in it: unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 69\), with those nested in it: sound} "contract_shadow" } }

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
