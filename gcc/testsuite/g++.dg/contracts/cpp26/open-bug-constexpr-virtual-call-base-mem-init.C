// Watch test for an open bug (GCC-758, stock GCC too; not a contracts bug):
// calling a member function of the class under construction from a
// mem-initializer for a base class is undefined ([class.base.init]/16),
// but constant evaluation accepts a virtual call there.  The non-virtual
// call is GCC-784 (open-bug-constexpr-call-in-base-mem-init.C).  When it is
// fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-virtual-call-base-mem-init.cpp.
// { dg-do compile { target c++26 } }

struct A { int a; constexpr A (int x) : a (x) {} };
struct B : A
{
  constexpr virtual int vf () const { return 1; }
  constexpr B () : A (vf ()) {}
};
constexpr int f () { B b; return b.a; }
static_assert (f () == 1);	// { dg-error "non-constant condition" "GCC-758" { xfail *-*-* } }
