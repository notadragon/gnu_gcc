// Watch test for an open bug (GCC-715, stock GCC too; not a contracts bug):
// rejects-valid.  A pointer to a member of C converted to a pointer to
// member of its base Bb ([expr.static.cast]/12) may be applied to an object
// that contains the member, here a C; constant evaluation rejects the use.
// When it is fixed, the xfailed dg-bogus lines report an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-ptrmem-cast-to-base.cpp, where Clang accepts
// it.
// { dg-do compile { target c++26 } }

struct Bb { int i; };
struct C : Bb { int m; };
constexpr int
f ()
{
  int Bb::*p = static_cast<int Bb::*> (&C::m);
  C c{};
  c.m = 5;
  return c.*p;			// { dg-bogus "not a constant expression" "GCC-715" { xfail *-*-* } }
}
static_assert (f () == 5);	// { dg-bogus "non-constant condition" "GCC-715" { xfail *-*-* } }
