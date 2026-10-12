// P3595 x templates: a template defined in a header and instantiated in the
// translation unit takes the semantic configured for the header's location
// -- where its contract assertions are written -- not the main file's, and
// so differs from a template with the same predicate defined in the main
// file.  The header's entry says observe, the catch-all ignore: every
// header instantiation reports its violation, none of the main file's does.
//
// Mirror: clang/test/Contracts/p3595-header-template.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-header-template.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include "p3595-header-template.h"

static int violations;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

template <class T> T tu_f (T x) pre (x > 0) { return x; }
template <class T> struct TuS { T g (T x) pre (x > 0) { return x; } };

int
main ()
{
  hdr_f (-1);
  hdr_f (-1.0);
  HdrS<int> ().g (-1);
  hdr_inline (-1);
  if (violations != 4)
    __builtin_abort ();
  tu_f (-1);
  tu_f (-1.0);
  TuS<int> ().g (-1);
  if (violations != 4)
    __builtin_abort ();
}
