// A [[noreturn]] function that returns has undefined behavior
// ([dcl.attr.noreturn]/2), so a constant evaluation that calls it is not a
// core constant expression.  GCC rejects it.
//
// The mirror of CLANG-711 in the llvm_llvm-project fork (stock Clang too,
// llvm#211272), where Clang accepts it; this pins GCC's behaviour on the
// same program.
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-noreturn-returns.cpp.
// { dg-do compile { target c++26 } }

[[noreturn]] constexpr void g () {}
constexpr bool f () { g (); return true; }	// { dg-error "call returns" }
static_assert (f ());	// { dg-error "non-constant condition" }
