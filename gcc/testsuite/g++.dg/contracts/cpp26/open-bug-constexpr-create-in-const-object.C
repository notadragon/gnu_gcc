// Watch test for an open bug (GCC-745, stock GCC too, PR119304; not a
// contracts bug): creating an object in the storage of a const complete
// object is undefined ([basic.life]/10), but constant evaluation accepts
// it.  When it is fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-create-in-const-object.cpp, where Clang
// rejects it.
// { dg-do compile { target c++26 } }

#include <memory>

struct B { int x; };
constexpr int
f ()
{
  const B b{1};
  std::construct_at (const_cast<B *> (&b), 2);
  return b.x;
}
static_assert (f () == 2);	// { dg-error "non-constant condition" "GCC-745" { xfail *-*-* } }
