// Watch test for an open bug (GCC-711, stock GCC too, PR125199; not a
// contracts bug): constant evaluation of a static_cast from void* whose
// operand is a POINTER_PLUS_EXPR (`a + 1`, not `&a[1]`) ICEs in the P2738
// diagnostic, which asserts an ADDR_EXPR.  The cast is not a constant
// expression (the pointee is an int, not an unsigned), so the right outcome
// is an error.  The constexpr variable is used, not a static_assert: a
// static_assert diagnoses first, and a release compiler then turns the ICE
// into "confused by earlier errors, bailing out", which dg-ice cannot see.
// When it is fixed, dg-ice reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-void-cast-pointer-plus.cpp, where Clang
// rejects the cast.
// { dg-do compile { target c++26 } }
// { dg-ice "GCC-711" }

constexpr bool
f ()
{
  int a[2]{};
  void *v = a + 1;
  unsigned *p = static_cast<unsigned *> (v);
  return p;
}
constexpr bool x = f ();
