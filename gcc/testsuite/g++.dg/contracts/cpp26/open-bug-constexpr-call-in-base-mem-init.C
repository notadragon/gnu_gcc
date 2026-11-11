// Watch test for an open bug (GCC-784, stock GCC too, PR126357; not a
// contracts bug): calling a member function of the class under
// construction, or applying dynamic_cast to `this`, in a mem-initializer
// for a base class is undefined ([class.base.init]/16), but constant
// evaluation accepts it.  The virtual call is GCC-758.  When it is fixed,
// the xfailed dg-error lines report an XPASS.
//
// Also the mirror of CLANG-713 in the llvm_llvm-project fork (the same
// shapes).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-call-in-base-mem-init.cpp and
// clang/test/Contracts/OpenBugs/constexpr-dynamic-cast-in-base-mem-init.cpp.
// { dg-do compile { target c++26 } }

struct A { int a; constexpr A (int x) : a (x) {} };
struct B : A
{
  constexpr int g () const { return 1; }
  constexpr B () : A (g ()) {}
};
static_assert ((B{}, true));	// { dg-error "non-constant condition" "GCC-784" { xfail *-*-* } }

struct P { bool p; constexpr P (bool x) : p (x) {} virtual void f () {} };
struct Q : P
{
  constexpr Q () : P (dynamic_cast<Q *> (this) != nullptr) {}
};
static_assert ((Q{}, true));	// { dg-error "non-constant condition" "GCC-784" { xfail *-*-* } }
