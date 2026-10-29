// Watch test for an open bug (GCC-763, stock GCC too; not a contracts bug):
// binding a reference through a past-the-end pointer is undefined
// ([expr.unary.op]/1: the pointer points to no object), but constant
// evaluation accepts it.  The address form `&*(a + 1)` is GCC-785, the
// dangling form GCC-786.  When it is fixed, the xfailed dg-error line
// reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-reference-through-invalid-pointer.cpp.
// { dg-do compile { target c++26 } }

constexpr bool
f ()
{
  int a[1] = {0};
  int *p = a + 1;
  int &r = *p;
  (void) r;
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-763" { xfail *-*-* } }
