// The P3100 null-dereference check tests the object pointer of `->*' with a
// pointer to data member ([expr.mptr.oper] makes np->*pm (*np).*pm), before
// the member's offset is added to it.
//
// Mirror: clang/test/Contracts/p3100-null-ptrmem.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <csetjmp>

static std::jmp_buf env;
static int caught = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++caught;
  std::longjmp (env, 1);  // do not continue into the faulting access
}

struct S { int a; int b; };
S *volatile np = nullptr;
int S::*volatile pm = &S::b;

__attribute__((noinline)) int ptm () { return np->*pm; }

int main ()
{
  if (setjmp (env) == 0)
    ptm ();
  if (caught != 1)
    __builtin_abort ();
}
