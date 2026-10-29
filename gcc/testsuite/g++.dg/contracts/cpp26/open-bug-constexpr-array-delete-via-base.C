// Watch test for an open bug (GCC-753, stock GCC too; not a contracts bug):
// delete[] through a pointer to a base class is undefined ([expr.delete]/3),
// but constant evaluation accepts it when the derived class has the size of
// the base (or the array has one element).  When it is fixed, the xfailed
// dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-array-delete-via-base.cpp.
// { dg-do compile { target c++26 } }

struct B { int i = 0; };
struct D : B {};		// same size as B
constexpr bool
f ()
{
  B *p = new D[2];
  delete[] p;
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-753" { xfail *-*-* } }
