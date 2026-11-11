// P4405, notadragon_wg21 DECISIONS.md E196 item 1: an analysis evaluation
// that is not proven is still a premise for the later ones on its path (a
// diagnose marker, like a check, lets what follows assume its predicate),
// so `pre (x > 3) pre (x > 2) pre (x > 1)' gives one warning, not three.
// But a proof resting on it is only conditional: under
// -fcontracts-prove-assume it never becomes an assumption, so the real
// code keeps `y > 2' (g2, g3), where a sound proof lets it fold (g4).
// At -O0, where only folding runs, there are no premises:
// p4405-premise-unproven-O0.C.
// Mirror: clang/test/Contracts/p4405-premise-unproven.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontracts-prove-assume" }
// { dg-additional-options "-fdump-tree-contract_shadow -fdump-tree-optimized" }

#include <contracts>
using namespace std::contracts::labels;

void f (int x) pre<diagnose> (x > 3) pre<diagnose> (x > 2) pre<diagnose> (x > 1);

void g1 (int y)
{
  f (y);
}
// Exactly one warning at the call.
// { dg-regexp {[^\n]*p4405-premise-unproven\.C:25:5: warning: contract assertion cannot be proven[^\n]*} }

int g2 (int y)
{
  contract_assert<diagnose> (y > 3);	// { dg-warning "cannot be proven" }
  contract_assert<diagnose> (y > 2);	// { dg-bogus "cannot be proven" }
  contract_assert<diagnose> (y > 1);	// { dg-bogus "cannot be proven" }
  return y > 2;
}

int g3 (int y)
{
  const int k = 1;
  contract_assert<diagnose> (k == 0);	// { dg-warning "always false" }
  contract_assert<diagnose> (y > 2);	// { dg-bogus "cannot be proven" }
  return y > 2;
}

int g4 (int y)
{
  if (y <= 2)
    __builtin_unreachable ();
  contract_assert<diagnose> (y > 2);
  return y > 2;
}

// { dg-final { scan-tree-dump-times "contract obligation \[0-9\]+ \\(line 25\\): conditional" 2 "contract_shadow" } }
// { dg-final { scan-tree-dump "contract obligation \[0-9\]+ \\(line 33\\): conditional" "contract_shadow" } }
// { dg-final { scan-tree-dump "contract obligation \[0-9\]+ \\(line 34\\): conditional" "contract_shadow" } }
// { dg-final { scan-tree-dump "contract obligation \[0-9\]+ \\(line 41\\): always false" "contract_shadow" } }
// { dg-final { scan-tree-dump "contract obligation \[0-9\]+ \\(line 42\\): conditional" "contract_shadow" } }
// { dg-final { scan-tree-dump "contract obligation \[0-9\]+ \\(line 50\\): sound" "contract_shadow" } }
// The real code: only g4's comparison folds.
// { dg-final { scan-tree-dump-times "return 1;" 1 "optimized" } }

// A -O0 build, at the default proof level, 2, gives the same diagnostics.
// { dg-final { contracts-same-diagnostics "" "-O0" } }
