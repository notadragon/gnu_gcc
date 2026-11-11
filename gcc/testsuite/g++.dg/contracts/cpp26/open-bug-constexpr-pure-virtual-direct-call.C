// Watch test for an open bug (GCC-760, stock GCC too; not a contracts bug):
// a virtual call to a pure virtual function from its class's constructor is
// undefined ([class.abstract]/6) even when the function has a definition,
// but constant evaluation calls the definition, A::f.  When it is fixed,
// the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-pure-virtual-direct-call.cpp.
// { dg-do compile { target c++26 } }

struct A
{
  constexpr virtual int f () const = 0;
  int v;
  constexpr A () : v (f ()) {}	// { dg-warning "pure virtual" }
};
constexpr int A::f () const { return 1; }
struct B : A { constexpr int f () const override { return 2; } };
constexpr int g () { B b; return b.v; }
static_assert (g () == 1);	// { dg-error "non-constant condition" "GCC-760" { xfail *-*-* } }
