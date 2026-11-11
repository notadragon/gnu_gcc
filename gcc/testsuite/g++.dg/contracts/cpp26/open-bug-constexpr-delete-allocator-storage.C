// Watch test for an open bug (GCC-764, stock GCC too; not a contracts bug):
// a delete-expression whose operand was not obtained from a non-array
// new-expression is undefined ([expr.delete]/2), here storage from
// std::allocator<int>::allocate, but constant evaluation accepts it.  When
// it is fixed, the xfailed dg-error line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-delete-allocator-storage.cpp.
// { dg-do compile { target c++26 } }

#include <memory>

constexpr bool
f ()
{
  std::allocator<int> a;
  int *p = a.allocate (1);
  delete p;
  return true;
}
static_assert (f ());	// { dg-error "non-constant condition" "GCC-764" { xfail *-*-* } }
