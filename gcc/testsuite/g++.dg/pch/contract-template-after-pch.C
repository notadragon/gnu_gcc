// Function templates declared in a precompiled header and instantiated
// after it keep their contract assertions, in constant evaluation (below)
// and at run time alike (GCC-646).  The table of contract specifiers used
// to be hashed on addresses, which writing the PCH changes, so a template's
// were found or lost depending on the build of the .gch; there are twelve
// templates here so that a regression cannot pass by chance.
//
// The violations are diagnosed at the predicate, in the header, so the
// dg-error lines name the lines of contract-template-after-pch.Hs.
//
// Mirror: clang/test/Contracts/pch-contracts-constexpr.cpp in the
// llvm_llvm-project fork.
// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce" }

#include "contract-template-after-pch.H"

static_assert (t0 (1) == 1);
static_assert (t0 (0) == 0);
// { dg-error "contract predicate is false" "t0 pre (header line 3)" { target *-*-* } 3 }
static_assert (t0 (9L) == 9);
// { dg-error "contract predicate is false" "t0 post (header line 4)" { target *-*-* } 4 }
static_assert (t1 (2) == 2);
static_assert (t1 (0) == 0);
// { dg-error "contract predicate is false" "t1 pre (header line 6)" { target *-*-* } 6 }
static_assert (t1 (10L) == 10);
// { dg-error "contract predicate is false" "t1 post (header line 7)" { target *-*-* } 7 }
static_assert (t2 (3) == 3);
static_assert (t2 (0) == 0);
// { dg-error "contract predicate is false" "t2 pre (header line 9)" { target *-*-* } 9 }
static_assert (t2 (11L) == 11);
// { dg-error "contract predicate is false" "t2 post (header line 10)" { target *-*-* } 10 }
static_assert (t3 (4) == 4);
static_assert (t3 (0) == 0);
// { dg-error "contract predicate is false" "t3 pre (header line 12)" { target *-*-* } 12 }
static_assert (t3 (12L) == 12);
// { dg-error "contract predicate is false" "t3 post (header line 13)" { target *-*-* } 13 }
static_assert (t4 (5) == 5);
static_assert (t4 (0) == 0);
// { dg-error "contract predicate is false" "t4 pre (header line 15)" { target *-*-* } 15 }
static_assert (t4 (13L) == 13);
// { dg-error "contract predicate is false" "t4 post (header line 16)" { target *-*-* } 16 }
static_assert (t5 (6) == 6);
static_assert (t5 (0) == 0);
// { dg-error "contract predicate is false" "t5 pre (header line 18)" { target *-*-* } 18 }
static_assert (t5 (14L) == 14);
// { dg-error "contract predicate is false" "t5 post (header line 19)" { target *-*-* } 19 }
static_assert (t6 (7) == 7);
static_assert (t6 (0) == 0);
// { dg-error "contract predicate is false" "t6 pre (header line 21)" { target *-*-* } 21 }
static_assert (t6 (15L) == 15);
// { dg-error "contract predicate is false" "t6 post (header line 22)" { target *-*-* } 22 }
static_assert (t7 (8) == 8);
static_assert (t7 (0) == 0);
// { dg-error "contract predicate is false" "t7 pre (header line 24)" { target *-*-* } 24 }
static_assert (t7 (16L) == 16);
// { dg-error "contract predicate is false" "t7 post (header line 25)" { target *-*-* } 25 }
static_assert (t8 (9) == 9);
static_assert (t8 (0) == 0);
// { dg-error "contract predicate is false" "t8 pre (header line 27)" { target *-*-* } 27 }
static_assert (t8 (17L) == 17);
// { dg-error "contract predicate is false" "t8 post (header line 28)" { target *-*-* } 28 }
static_assert (t9 (10) == 10);
static_assert (t9 (0) == 0);
// { dg-error "contract predicate is false" "t9 pre (header line 30)" { target *-*-* } 30 }
static_assert (t9 (18L) == 18);
// { dg-error "contract predicate is false" "t9 post (header line 31)" { target *-*-* } 31 }
static_assert (t10 (11) == 11);
static_assert (t10 (0) == 0);
// { dg-error "contract predicate is false" "t10 pre (header line 33)" { target *-*-* } 33 }
static_assert (t10 (19L) == 19);
// { dg-error "contract predicate is false" "t10 post (header line 34)" { target *-*-* } 34 }
static_assert (t11 (12) == 12);
static_assert (t11 (0) == 0);
// { dg-error "contract predicate is false" "t11 pre (header line 36)" { target *-*-* } 36 }
static_assert (t11 (20L) == 20);
// { dg-error "contract predicate is false" "t11 post (header line 37)" { target *-*-* } 37 }
