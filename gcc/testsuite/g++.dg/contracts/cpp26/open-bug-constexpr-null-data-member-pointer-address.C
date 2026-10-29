// Watch test for an open bug (GCC-756, stock GCC too; not a contracts bug):
// `s.*pm` with a null pointer to data member is undefined
// ([expr.mptr.oper]/6), but constant evaluation accepts it when only its
// address is taken.  When it is fixed, the xfailed dg-error line reports
// an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-null-data-member-pointer-address.cpp.
// { dg-do compile { target c++26 } }

struct S { int m = 1; };
constexpr bool
f ()
{
  int S::*pm = nullptr;
  S s;
  int *p = &(s.*pm);
  return p != nullptr;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-756" { xfail *-*-* } }
