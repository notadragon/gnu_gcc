// Watch test for an open bug (GCC-747, stock GCC too, PR93096; not a
// contracts bug): a dynamic_cast during construction whose operand is not
// the object under construction or one of its bases, here reaching Bx
// through A, is undefined ([class.cdtor]/6, the standard's own example),
// but constant evaluation accepts it.  When it is fixed, the xfailed
// dg-error line reports an XPASS.
//
// Also the mirror of CLANG-714 in the llvm_llvm-project fork (the same
// program).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-cdtor-dynamic-cast.cpp.
// { dg-do compile { target c++26 } }

struct V { virtual void f () {} };
struct A : virtual V {};
struct Bx : virtual V
{
  bool ok;
  constexpr Bx (A *a) : ok (dynamic_cast<Bx *> (a) != nullptr) {}
};
struct D : A, Bx { constexpr D () : Bx (this) {} };
static_assert ((D{}, true));	// { dg-error "non-constant condition" "GCC-747" { xfail *-*-* } }
