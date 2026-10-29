// Watch test for an open bug (GCC-743, stock GCC too, PR121250, PR105277;
// not a contracts bug): applying a pointer to member to an object that does
// not contain the member is undefined ([expr.mptr.oper]/4), but constant
// evaluation accepts it when no data is read, as for a member function.
// When it is fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-ptrmem-object-lacks-member.cpp, where Clang
// rejects it.
// { dg-do compile { target c++26 } }

struct B { int dummy = 0; };
struct D : B { int x = 0; constexpr int h () const { return 2; } };
constexpr int
f ()
{
  int (B::*pb) () const = static_cast<int (B::*) () const> (&D::h);
  B b;
  return (b.*pb) ();		// b does not contain D::h
}
static_assert (f () == 2);	// { dg-error "non-constant condition" "GCC-743" { xfail *-*-* } }
