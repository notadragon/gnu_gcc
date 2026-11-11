// Watch test for an open bug (GCC-786, stock GCC too; not a contracts bug):
// binding a reference through a pointer to an automatic object whose
// scope has ended is an indirection through a pointer that points to no
// object, which is undefined ([expr.unary.op]/1, [basic.stc]/4), but
// constant evaluation accepts it; reading through the pointer is rejected
// (PR70331).  The past-the-end form is GCC-763.  When it is fixed, the
// xfailed dg-error line reports an XPASS.
//
// Also the mirror of CLANG-717 in the llvm_llvm-project fork (the same
// program).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-reference-through-dangling-pointer.cpp.
// { dg-do compile { target c++26 } }

constexpr bool
t ()
{
  int *p;
  { int y = 1; p = &y; }
  int &r = *p;
  (void) r;
  return true;
}
static_assert (t ());	// { dg-error "non-constant condition" "GCC-786" { xfail *-*-* } }

// Control, which passes: reading through it is rejected.
constexpr int
u ()
{
  int *p;
  { int y = 1; p = &y; }
  return *p;
}
static_assert (u () == 1);	// { dg-error "non-constant condition" }
// { dg-error "outside its lifetime" "" { target *-*-* } .-3 }
