// Watch test for an open bug (GCC-759, stock GCC too; not a contracts bug):
// converting `this` to a virtual base A of DBase before DBase's
// construction has begun is undefined ([class.cdtor]/3), but constant
// evaluation accepts it.  When it is fixed, the xfailed dg-error line
// reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-cdtor-convert-virtual-base.cpp.
// { dg-do compile { target c++26 } }

struct A { int tag = 1; };
struct CBase : virtual A {};
struct DBase : virtual A { A *p; constexpr DBase (A *a) : p (a) {} };
struct E : CBase, DBase
{
  constexpr E () : DBase (this) {}	// DBase not yet constructed
};
constexpr int t () { E e; return e.p == static_cast<A *> (&e); }
static_assert (t ());	// { dg-error "non-constant condition" "GCC-759" { xfail *-*-* } }
