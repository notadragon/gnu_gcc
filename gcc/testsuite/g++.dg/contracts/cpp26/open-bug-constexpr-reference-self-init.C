// Watch test for an open bug (GCC-746, stock GCC too, PR119339; not a
// contracts bug): a reference used in its own initializer refers to no
// object ([basic.life], [dcl.ref]), which is undefined, but constant
// evaluation accepts it.  When it is fixed, the xfailed dg-error line
// reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-reference-self-init.cpp, where Clang rejects
// it.
// { dg-do compile { target c++26 } }

constexpr int &id (int &r) { return r; }
constexpr bool f () { int &r = id (r); (void) r; return true; }
static_assert (f ());	// { dg-error "non-constant condition" "GCC-746" { xfail *-*-* } }
