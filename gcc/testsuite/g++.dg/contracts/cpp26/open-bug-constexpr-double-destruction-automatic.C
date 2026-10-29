// Watch test for an open bug (GCC-714, stock GCC too, PR102284 comment 2;
// not a contracts bug): an automatic object whose destructor is called
// explicitly is destroyed again at the end of its scope ([basic.life]/9),
// which is undefined.  When the destructor does not touch the object,
// constant evaluation accepts it.  When it is fixed, the xfailed dg-error
// line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-double-destruction-automatic.cpp, where Clang
// rejects it.
// { dg-do compile { target c++26 } }

struct X { constexpr ~X () {} };
constexpr bool
f ()
{
  X x;
  x.~X ();
  return true;		// x is destroyed again at scope exit
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-714" { xfail *-*-* } }
