// P3100: a throwing handler under the front-end array-bounds
// (ub:expr.add.out.of.bounds.known) and division-overflow
// (ub:expr.mul.representable.type.result) checks, in functions that are not
// noexcept: the exception propagates out of the checked operation and is
// caught by the caller, under observe and enforce alike -- the two
// front-end checks that had no throwing-handler test (division and
// remainder by zero, shift, enum and float conversion, flow-off and pure
// virtual calls each have one).
//
// Mirror: clang/test/Contracts/Runnable/p3100-throw-bounds-divoverflow.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-throw-bounds-divoverflow.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E { int tag; };
static int calls;
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++calls;
  if (v.kind () != std::contracts::assertion_kind::implicit)
    __builtin_abort ();
  throw E{ 7 };
}

int g[4];
constexpr int kMin = -__INT_MAX__ - 1;
namespace obs_ns {
  int rd (int i) { return g[i]; }
  int divi (int a, int b) { return a / b; }
  int modi (int a, int b) { return a % b; }
}
namespace enf_ns {
  int rd (int i) { return g[i]; }
  int divi (int a, int b) { return a / b; }
}

__attribute__ ((noinline)) int opaque (int x) { return x; }

template <class F>
static int
caught (F f)
{
  try { f (); } catch (E &e) { return e.tag == 7; }
  return 0;
}

int
main ()
{
  int n = 0;
  n += caught ([] { obs_ns::rd (opaque (4)); });
  n += caught ([] { obs_ns::divi (kMin, opaque (-1)); });
  n += caught ([] { obs_ns::modi (kMin, opaque (-1)); });
  n += caught ([] { enf_ns::rd (opaque (-1)); });
  n += caught ([] { enf_ns::divi (kMin, opaque (-1)); });
  if (n != 5 || calls != 5)
    __builtin_abort ();
  // In range: no handler.
  obs_ns::rd (opaque (3));
  enf_ns::divi (kMin, opaque (1));
  if (calls != 5)
    __builtin_abort ();
}
