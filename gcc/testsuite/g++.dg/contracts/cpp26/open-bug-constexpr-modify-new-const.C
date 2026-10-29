// Watch test for an open bug (GCC-754, stock GCC too; not a contracts bug):
// modifying an object created by `new const int` is undefined
// ([dcl.type.cv]/4), but constant evaluation accepts it.  When it is
// fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-modify-new-const.cpp.
// { dg-do compile { target c++26 } }

constexpr int
f ()
{
  const int *p = new const int (3);
  *const_cast<int *> (p) = 4;
  int r = *p;
  delete p;
  return r;
}
static_assert (f () == 4);	// { dg-error "non-constant condition" "GCC-754" { xfail *-*-* } }
