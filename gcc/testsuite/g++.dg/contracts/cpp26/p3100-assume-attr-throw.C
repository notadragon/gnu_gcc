// P3100: [[assume]] with a THROWING violation handler in a non-noexcept
// function -- the exception propagates out and is caught by the caller under
// both observe and enforce; and when it unwinds, in-scope automatic objects are
// destroyed (the handler call is a proper unwinding call).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-assume-attr-throw.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct E { int tag; };
struct S { static int dtors; ~S () { ++dtors; } };
int S::dtors = 0;
static int calls = 0;
void handle_contract_violation (const std::contracts::contract_violation &) {
  ++calls;
  throw E{7};
}

namespace obs_ns     { int f (int x) { [[assume (x > 0)]]; return x; } }   // observe
namespace enf_ns     { int f (int x) { [[assume (x > 0)]]; return x; } }   // enforce
namespace obs_clean  { int f (int x) { S s; [[assume (x > 0)]]; return x; } }
namespace enf_clean  { int f (int x) { S s; [[assume (x > 0)]]; return x; } }

int main () {
  int caught = 0;
  try { obs_ns::f (-1); } catch (E &e) { if (e.tag == 7) ++caught; }
  try { enf_ns::f (-1); } catch (E &e) { if (e.tag == 7) ++caught; }
  if (caught != 2 || calls != 2) __builtin_abort ();

  // A throwing reaction unwinds the scope containing the [[assume]], so the
  // in-scope automatic object's destructor runs -- under observe and enforce.
  try { obs_clean::f (-1); } catch (E &) {}
  if (S::dtors != 1) __builtin_abort ();
  try { enf_clean::f (-1); } catch (E &) {}
  if (S::dtors != 2) __builtin_abort ();
}
