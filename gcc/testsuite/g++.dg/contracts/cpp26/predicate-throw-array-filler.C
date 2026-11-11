// A predicate that can throw only through an initializer list's array filler
// (the default constructor of the array elements it does not initialize
// explicitly) reports the exception to the violation handler as an
// evaluation_exception, rather than letting it escape the function.  It does
// not depend on noexcept getting the filler right (GCC-682).
//
// Mirror: clang/test/Contracts/predicate-throw-array-filler.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int handled = 0;

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (v.detection_mode ()
      == std::contracts::detection_mode::evaluation_exception)
    ++handled;
}

struct S { S () { throw 2; } };
struct A { S s[2]; };
bool f (int x) pre (((void) A{}, x > 0)) { return true; }

int
main ()
{
  int escaped = 0;
  try { f (1); } catch (...) { escaped = 1; }
  if (handled != 1 || escaped != 0)
    __builtin_abort ();
}
