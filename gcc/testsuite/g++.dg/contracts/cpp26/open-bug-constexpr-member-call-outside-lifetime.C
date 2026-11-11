// Watch test for an open bug (GCC-751, stock GCC too; not a contracts bug):
// calling a member function on an object outside its lifetime is
// undefined ([basic.life]/7), but constant evaluation accepts it when the
// function reads nothing.  When it is fixed, the xfailed dg-error line
// reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-member-call-outside-lifetime.cpp.
// { dg-do compile { target c++26 } }

#include <memory>

struct S
{
  int i = 0;
  constexpr int g () const { return 7; }
  constexpr ~S () {}
};
constexpr int
f ()
{
  S s;
  s.~S ();
  int r = s.g ();	// s is outside its lifetime
  std::construct_at (&s);
  return r;
}
static_assert (f () == 7);	// { dg-error "non-constant condition" "GCC-751" { xfail *-*-* } }
