// Watch test for an open bug (GCC-716, stock GCC too; not a contracts bug):
// a static_cast of an out-of-range floating value to an unfixed enumeration
// is rightly rejected in a constant expression ([expr.static.cast]/10: the
// value 4 is outside the range [0, 3] of A), but the diagnostic names
// neither the enumeration nor the range: "'(unsigned int)4.0e+0' is not a
// constant expression".  The xfailed dg-message reports an XPASS once a
// diagnostic mentions the enumeration or its range.
//
// Also the mirror of CLANG-718 in the llvm_llvm-project fork (Clang accepts
// the cast): the rejection, which passes, is pinned here.
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-enum-floating-cast.cpp.
// { dg-do compile { target c++26 } }

enum A { a0, a1, a2, a3 };
constexpr A x = static_cast<A> (4.0);	// { dg-error "not a constant expression" }
// { dg-message "(enumeration|range)" "GCC-716" { xfail *-*-* } .-1 }

// Control, which passes: an in-range value.
constexpr A y = static_cast<A> (3.0);
static_assert (y == a3);
