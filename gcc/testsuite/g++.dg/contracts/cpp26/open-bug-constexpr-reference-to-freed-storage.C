// Watch test for an open bug (GCC-761, stock GCC too; not a contracts bug):
// naming a reference whose referent's storage has been deallocated is
// undefined ([basic.compound]/6, [expr.type]), but constant evaluation
// accepts it when nothing is read.  When it is fixed, the xfailed dg-error
// line reports an XPASS.
//
// Also the mirror of CLANG-715 in the llvm_llvm-project fork (the same
// shape).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-reference-to-freed-storage.cpp.
// { dg-do compile { target c++26 } }

constexpr bool
f ()
{
  int *ip = new int (1);
  int &i = *ip;
  delete ip;
  int &j = i;
  (void) j;
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-761" { xfail *-*-* } }
