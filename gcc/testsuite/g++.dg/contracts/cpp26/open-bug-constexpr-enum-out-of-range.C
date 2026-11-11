// Watch test for an open bug (GCC-741, stock GCC too, PR95701; not a
// contracts bug): a static_cast of an integral value outside the range of
// an unfixed enumeration is undefined ([expr.static.cast]/9), but constant
// evaluation accepts it.  When it is fixed, the xfailed dg-error line
// reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-enum-out-of-range.cpp, where Clang rejects
// it.
// { dg-do compile { target c++26 } }

enum A { a0, a1, a2, a3 };			// values [0, 3]
constexpr A x = static_cast<A> (4);	// { dg-error "(not a constant expression|range)" "GCC-741" { xfail *-*-* } }
