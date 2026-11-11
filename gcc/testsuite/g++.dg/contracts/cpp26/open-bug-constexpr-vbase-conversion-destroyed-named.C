// Watch test for an open bug (GCC-750, stock GCC too; not a contracts bug):
// converting a named object outside its lifetime to a virtual base is
// undefined ([basic.life]/8), but constant evaluation accepts it.  When it
// is fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-vbase-conversion-destroyed-named.cpp.
// { dg-do compile { target c++26 } }

#include <memory>

struct V { int v = 1; };
struct D : virtual V { constexpr ~D () {} };
constexpr bool
f ()
{
  D d;
  d.~D ();
  V &r = d;		// d is outside its lifetime
  (void) r;
  std::construct_at (&d);
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-750" { xfail *-*-* } }
