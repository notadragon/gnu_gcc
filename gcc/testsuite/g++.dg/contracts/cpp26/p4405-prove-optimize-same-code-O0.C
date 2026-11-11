// D4405's proof optimization level (DECISIONS.md E162-E168, E166): a -O0
// compilation makes proof shadows at the proof level, 2 by default, and its
// code is still the same as at proof level 0 and without the shadows, with
// every evaluation diagnose or ignore.  -O0 removes nothing the front end
// leaves, so the regions of the evaluations must leave no trace in the real
// functions: here where a region is all a statement holds -- the arm of an
// `if', of a short-circuit condition, of a nested `if', a `switch' case, a
// loop body (of a `for' and of a `while') -- whatever the condition reads
// (parameters, memory, a call, a volatile object, which the code evaluates
// either way), where a predicate needs temporaries (a class with a
// destructor, a lambda), and loads of a `bool' (f15), whose range the
// proofs know at the proof level and -O0 code does not.  The
// dg-final code compiles the file with each set of options and compares
// the object code; -Og too, which removes little; with every strategy
// given explicitly at proof level 0 too, where the shadows run at level 0
// (E208 (a)).  Without -g: f5, f8, f9,
// f10, f11 and f13 still differ in their debug information (GCC-998,
// open-bug-p4405-shadow-debug-info-residue.C); the other shapes are
// compared with -g in p4405-shadow-debug-info-statements.C.
//
// Mirror: clang/test/Contracts/p4405-prove-optimize-same-code-O0.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fno-contracts-prove-assume -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven" }

struct S { int x; volatile int v; };
struct T { int n; T (int n) : n (n) {} ~T (); bool ok () const { return n > 0; } };
int g (int);

int f1 (int i, int n) { if (i < n) contract_assert (n > 0); return 1; }
int f2 (S *p, int n) { if (p->x < n) contract_assert (n > 0); return 2; }
int f3 (int i, int n) { if (g (i) < n) contract_assert (n > 0); return 3; }
int f4 (S *p, int n) { if (p->v < n) contract_assert (n > 0); return 4; }
int f5 (int i, int n)
{
  if (i < n)
    contract_assert (n > 0);
  else
    contract_assert (n < 0);
  return 5;
}
int f6 (S *p, int i, int n) { if (i < n && p->x > 0) contract_assert (n > 0); return 6; }
int f7 (int i, int n) { if (i < n) { if (n > 3) contract_assert (i > 0); } return 7; }
int f8 (int i, int n)
{
  switch (i)
    {
    case 1:
      contract_assert (n > 0);
      break;
    default:
      break;
    }
  return 8;
}
int f9 (const int *p, int n) { for (int j = 0; j < n; ++j) contract_assert (p[j] > 0); return 9; }
int f10 (int n) { contract_assert (T (n).ok ()); return 10; }
int f11 (int n) { contract_assert ([&] { return n > 0; } ()); return 11; }
int f12 (int i, int n) pre (i < n) post (r : r > 0) { if (i > 0) contract_assert (n > 1); return 12; }
int f13 (int i, int n) { if (i < n) contract_assert (g (n) > 0); return 13; }
int f14 (int i, int n) { while (i < n) { contract_assert (i < n); ++i; } return i; }
bool f15 (bool b, int *p) { contract_assert (!b || *p > 0); return b; }

// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" "" "-fcontracts-prove-optimize=0" "-fno-contracts-prove-shadow" "-fcontract-evaluation-semantic=ignore" "-fcontracts-prove-assume" "-fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -fcontracts-prove-assume" } }
// { dg-final { contracts-same-object-code "-Og -fcontract-evaluation-semantic=ignore -fcontracts-prove-optimize=0" "-Og" "-Og -fcontracts-prove-optimize=0" "-Og -fcontract-evaluation-semantic=ignore" "-Og -fcontracts-prove-optimize=0 -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises" } }
