// Watch test for an open bug (GCC-742, stock GCC too, PR109243; not a
// contracts bug): a static_cast of a pointer to member to a base class that
// does not contain the member is undefined ([expr.static.cast]/12), but
// constant evaluation accepts it.  When it is fixed, the xfailed dg-error
// line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-ptrmem-cast-unrelated-base.cpp, where Clang
// rejects it.
// { dg-do compile { target c++26 } }

struct A { int a; };
struct B { int b; };
struct D : A, B {};
constexpr bool
f ()
{
  int D::*pd = &A::a;
  int B::*pb = static_cast<int B::*> (pd);	// B does not contain A::a
  return pb != nullptr;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-742" { xfail *-*-* } }
