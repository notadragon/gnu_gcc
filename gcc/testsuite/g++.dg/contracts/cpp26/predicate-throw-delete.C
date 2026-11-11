// A predicate that can throw only through the destructor a delete-expression
// invokes reports the exception to the violation handler as an
// evaluation_exception, rather than letting it escape the function.
//
// Mirror: clang/test/Contracts/predicate-throw-delete.cpp in the llvm_llvm-project fork.
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

struct D { ~D () noexcept (false) { throw 1; } };
bool f (D *p) pre ((delete p, true)) { return true; }

int
main ()
{
  int escaped = 0;
  try { f (new D); } catch (...) { escaped = 1; }
  if (handled != 1 || escaped != 0)
    __builtin_abort ();
}
