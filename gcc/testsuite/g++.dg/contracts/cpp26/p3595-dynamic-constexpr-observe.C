// P3595: a dynamic entry is never consulted during constant evaluation; its
// "semantic" is what constant evaluation uses.  Here that semantic is
// observe: a false predicate is a warning, not an error, and the call is
// still a constant expression -- whatever the selector would return at run
// time (enforce here, which would make it an error), and whatever the
// command line says.  p3595-dynamic-constexpr.C covers an "enforce"
// semantic, p3595-dynamic-nodefault-constexpr-observe.C an entry with no
// semantic at all.
//
// Mirror: clang/test/Contracts/p3595-dynamic-constexpr-observe.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-constexpr-observe.json" }

#include <contracts>

std::contracts::evaluation_semantic
p3595_sel_ceo ()
{
  return std::contracts::evaluation_semantic::enforce;
}

constexpr int g (int x) pre (x > 0) { return x; } // { dg-warning "contract predicate is false in constant expression" }
constexpr int bad = g (-1);
constexpr int ok = g (1);
static_assert (bad == -1 && ok == 1);
