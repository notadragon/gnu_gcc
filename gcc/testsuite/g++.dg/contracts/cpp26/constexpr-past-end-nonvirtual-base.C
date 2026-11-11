// Converting a past-the-end pointer to a non-virtual base class is a
// constant expression: the conversion needs no object, only an offset
// ([conv.ptr]/3).  GCC accepts it.
//
// The mirror of CLANG-689 in the llvm_llvm-project fork (stock Clang too,
// llvm#43201), where Clang rejects it; this pins GCC's behaviour on the
// same program.
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-past-end-nonvirtual-base.cpp.
// { dg-do compile { target c++26 } }

struct B { int b = 1; };
struct NV : B { int x = 2; };
constexpr bool
t ()
{
  NV ds[1];
  NV *p = ds + 1;
  B *b = p;
  return b != nullptr;
}
static_assert (t ());
