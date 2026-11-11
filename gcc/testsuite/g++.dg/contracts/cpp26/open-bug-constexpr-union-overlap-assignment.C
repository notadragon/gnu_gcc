// Watch test for an open bug (GCC-762, stock GCC too; not a contracts bug):
// assigning from an object that inexactly overlaps the target is undefined
// ([expr.assign]/8), here two members of a union, but constant evaluation
// accepts it.  When it is fixed, the xfailed dg-error line reports an
// XPASS.
//
// Also the mirror of CLANG-716 in the llvm_llvm-project fork (the same
// program).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-union-overlap-assignment.cpp.
// { dg-do compile { target c++26 } }

union U { int i; short s; };
constexpr int
f ()
{
  U u;
  u.s = 1;
  u.i = u.s;
  return u.i;
}
static_assert (f () == 1);	// { dg-error "non-constant condition" "GCC-762" { xfail *-*-* } }
