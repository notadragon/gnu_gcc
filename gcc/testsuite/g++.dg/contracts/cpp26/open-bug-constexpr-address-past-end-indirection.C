// Watch test for an open bug (GCC-785, stock GCC too; not a contracts bug):
// `*(a + 1)` on a one-element array is an indirection through a pointer
// that points to no object, which is undefined ([expr.unary.op]/1) even
// when only its address is taken, but constant evaluation accepts
// `&*(a + 1)`.  Binding a reference to it is GCC-763.  When it is fixed,
// the xfailed dg-error line reports an XPASS.
//
// Also the mirror of CLANG-712 in the llvm_llvm-project fork (the same
// program; Clang accepts it too).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-address-past-end-indirection.cpp.
// { dg-do compile { target c++26 } }

constexpr bool
f ()
{
  int a[1] = {0};
  int *p = &*(a + 1);
  return p != nullptr;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-785" { xfail *-*-* } }

// Control, which passes: reading through it is rejected.
constexpr int g () { int a[1] = {0}; return *(a + 1); }
static_assert (g () == 0);	// { dg-error "non-constant condition" }
// { dg-error "(array subscript|outside the bounds)" "" { target *-*-* } .-2 }
