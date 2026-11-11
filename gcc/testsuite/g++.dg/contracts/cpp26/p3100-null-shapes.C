// The P3100 null-dereference check fires for every spelling of a dereference
// of a null pointer -- member access through `->' (including `np->arr[i]' and
// `&np->b'), a pointer subscript, a virtual call, and a non-virtual member
// call even when optimization could see `this' as non-null -- at -O0 and at
// -O2; this tests -O2.  The `->*' spelling is tested separately
// (p3100-null-ptrmem.C).
//
// Mirror: clang/test/Contracts/p3100-null-member-access.cpp,
// p3100-null-pointer-subscript.cpp, p3100-null-virtual-call.cpp and
// p3100-null-member-call-optimized.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-O2 -fcontracts -fcontracts-p3100 -fcontracts-p4298" }
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

struct S
{
  int a;
  int b;
  int arr[4];
  void f () {}
  virtual void v () {}
  int g () { return b; }
};
S *volatile np = nullptr;

__attribute__((noinline)) int member () { return np->b; }
__attribute__((noinline)) int arrmem ()
{
  volatile int i = 2;
  return np->arr[i];
}
__attribute__((noinline)) int arrow_addr () { return &np->b != (int *) 0; }
__attribute__((noinline)) int subscript ()
{
  int *volatile p = nullptr;
  return p[3];
}
__attribute__((noinline)) int vcall () { np->v (); return 0; }
__attribute__((noinline)) int mcall () { np->f (); return 0; }
__attribute__((noinline)) int mcall_reads () { return np->g (); }

static int expected = 0;
static void run (int (*fn) ())
{
  ++expected;
  if (setjmp (env) == 0)
    fn ();
}

int main ()
{
  run (member);
  run (arrmem);
  run (arrow_addr);
  run (subscript);
  run (vcall);
  run (mcall);
  run (mcall_reads);
  if (caught != expected)
    __builtin_abort ();
}
