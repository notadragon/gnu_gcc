// D4405's proof optimization level (DECISIONS.md E162-E168, E166): in a
// -O0 build, at the default proof level, 2, the proof shadows inline the
// inline members of a class template whose instantiation is declared by
// `extern template' -- bodies a -O0 compilation otherwise never analyzes
// -- so f's assertion, which needs S<int>::fix inlined, is proven as at
// -O2.  The bodies are kept only for the shadows: S<int>::fix and helper
// <int>, which only it calls, are not emitted, and the object code is the
// same as without the shadows and at proof level 0, where the assertion
// is not proven.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }

#include <contracts>
using namespace std::contracts::labels;

template <class T> inline T helper (T v) { return v < 0 ? 0 : v; }
template <class T> struct S { void fix (T &); };
template <class T> inline void S<T>::fix (T &v) { v = helper (v); }
extern template struct S<int>;

void f (int v)
{
  S<int> s;
  s.fix (v);
  contract_assert<diagnose> (v >= 0);	// { dg-bogus "cannot be proven" }
}

// { dg-final { scan-assembler-not "_ZN1SIiE3fixERi:" } }
// { dg-final { scan-assembler-not "_Z6helperIiET_S0_:" } }
// { dg-final { contracts-same-diagnostics "-O2" "" } }
// { dg-final { contracts-same-object-code "-fcontracts-prove-optimize=0 -Wno-contract-unproven" "-Wno-contract-unproven" "-fno-contracts-prove-shadow -Wno-contract-unproven" } }
