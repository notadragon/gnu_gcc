// Watch test for an open bug (GCC-748, stock GCC too, PR87275; not a
// contracts bug): an unsequenced modification and read of the same scalar
// is undefined ([intro.execution]/10), but constant evaluation accepts it.
// When it is fixed, the xfailed dg-error line reports an XPASS.
//
// Also the mirror of CLANG-710 in the llvm_llvm-project fork (the same
// program; Clang accepts it too).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-unsequenced-modification.cpp.
// { dg-do compile { target c++26 } }

constexpr int f () { int i = 0; i = i++ + i; return i; }
static_assert (f () >= 0);	// { dg-error "non-constant condition" "GCC-748" { xfail *-*-* } }
