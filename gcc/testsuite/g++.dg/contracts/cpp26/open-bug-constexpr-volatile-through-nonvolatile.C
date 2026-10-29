// Watch test for an open bug (GCC-744, stock GCC too, PR126359; not a
// contracts bug): reading a volatile object through a non-volatile glvalue
// is undefined ([dcl.type.cv]/5), but constant evaluation accepts it.
// When it is fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-volatile-through-nonvolatile.cpp, where Clang
// rejects it.
// { dg-do compile { target c++26 } }

constexpr int
f ()
{
  volatile int v = 1;
  return const_cast<int &> (v);
}
static_assert (f () == 1);	// { dg-error "non-constant condition" "GCC-744" { xfail *-*-* } }
