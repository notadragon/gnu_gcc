// Contract assertions declared in a precompiled header are checked in
// constant evaluation: a function's, a constructor's, a member function's
// precondition and contract_assert, and a lambda's.  (EDG shape:
// pch/constexpr.)  The violations are diagnosed at the predicate, in the
// header, so the dg-error lines name the line of contract-constexpr.Hs.
//
// A function template declared in the PCH and instantiated after it, and a
// redeclaration after the PCH (GCC-646, GCC-647), are
// contract-template-after-pch.C and contract-redecl-after-pch.C.
//
// Mirror: clang/test/Contracts/pch-contracts-constexpr.cpp in the
// llvm_llvm-project fork.
// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce" }

#include "contract-constexpr.H"

static_assert (cf (1) == 1);
static_assert (cf (0) == 0);
// { dg-error "contract predicate is false" "cf (header line 2)" { target *-*-* } 2 }
static_assert (S (5).v == 5);
static_assert (S (0).v == 0);
// { dg-error "contract predicate is false" "S::S (header line 4)" { target *-*-* } 4 }
static_assert (S (5).mf (4) == 4);
static_assert (S (5).mf (6) == 6);
static_assert (S (5).mf (2) == 2);
// { dg-error "contract predicate is false" "S::mf pre (header line 5)" { target *-*-* } 5 }
// { dg-error "contract predicate is false" "S::mf assert (header line 6)" { target *-*-* } 6 }
static_assert (lam (1) == 1);
static_assert (lam (0) == 0);
// { dg-error "contract predicate is false" "lambda (header line 9)" { target *-*-* } 9 }
