// Mirror of an open Clang bug (CLANG-39 in that fork's bug-reports/; stock
// Clang has it, no contracts involved): a requires-clause may only appear on
// a declarator that declares a templated function ([dcl.decl.general]).
// Clang accepted `typedef int F (int) requires true;', and the same as a
// member typedef of a class template; GCC diagnoses both, and this pins
// that (GCC-662).
//
// Mirror: clang/test/CXX/dcl.decl/dcl.decl.general/p4-20.cpp in the
// llvm_llvm-project fork, where the fork fixed CLANG-39.
// { dg-do compile { target c++20 } }

typedef int F (int) requires true; // { dg-error "requires-clause on typedef" }
template <typename T> struct TD
{
  typedef void G () requires true; // { dg-error "requires-clause on typedef" }
  typedef void H () requires (sizeof (T) > 1); // { dg-error "requires-clause on typedef" }
};
TD<int> td;
