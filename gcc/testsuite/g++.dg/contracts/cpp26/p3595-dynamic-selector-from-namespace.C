// P3595: a "C++"-linkage selector name is qualified from the global
// namespace, wherever the checked function is.  The check is built while the
// function's own namespace (and, for a member function defined in its class,
// the class) is still the current scope; resolving the name from there put
// the synthesized selector, and its weak default, in n1::lib::detail
// (GCC-655: a checking build asserted instead).  No selector is declared, so
// every check calls the one synthesized lib::detail::sel ().
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-selector-from-namespace.json" }

namespace n1 { void f (int x) pre (x > 0) {} }
namespace n2 { namespace n3 { void g (int x) pre (x > 0) {} } }
namespace n4 { struct S { void h (int x) pre (x > 0) {} }; }
void use (n4::S s) { s.h (1); }

// { dg-final { scan-assembler "\[ \t\]_ZN3lib6detail3selEv\[^A-Za-z0-9_\]" } }
// { dg-final { scan-assembler-not "_ZN2n13lib" } }
// { dg-final { scan-assembler-not "_ZN2n22n33lib" } }
// { dg-final { scan-assembler-not "_ZN2n43lib" } }
