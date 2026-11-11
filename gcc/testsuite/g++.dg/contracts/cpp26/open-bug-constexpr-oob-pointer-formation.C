// Watch test for an open bug (GCC-739, stock GCC too, PR70151, PR111242;
// not a contracts bug): forming a pointer more than one past the end of an
// array is undefined ([expr.add]/4.3), but constant evaluation accepts it
// when the pointer is brought back into range before it is used (only
// subscripts and reads are rejected).  When it is fixed, the xfailed
// dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-oob-pointer-formation.cpp, where Clang
// rejects it.
// { dg-do compile { target c++26 } }

constexpr int
f ()
{
  int a[3] = {1, 2, 3};
  int *p = a + 4;		// beyond one past the end
  return *(p - 4);
}
static_assert (f () == 1);	// { dg-error "non-constant condition" "GCC-739" { xfail *-*-* } }
