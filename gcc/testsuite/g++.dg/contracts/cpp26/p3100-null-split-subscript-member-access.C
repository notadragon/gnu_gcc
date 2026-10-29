// P3100: the address of a member of an element, &q[i].n, forms an address
// only, so its null check is the .indirection leaf of
// ub:expr.unary.dereference.nullptr, as for &q->n: with only .access
// configured it is not reported, and with only .indirection a null `q` is
// reported for i == 0.  GCC checks the element's address `q + i`, so a
// null `q` with i != 0 is not reported (Clang checks `q` itself and reports
// it: divergence GCC-794, CLANG-745; DECISIONS.md E83 is open).  This run
// is configured with .access.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-split-subscript-member.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv (int) { siglongjmp (env, 1); }

#define RUN(name, ...)						\
  do								\
    {								\
      reports = 0;						\
      if (!sigsetjmp (env, 1))					\
	(void) (__VA_ARGS__);					\
      std::printf ("%s %d\n", #name, reports);			\
    }								\
  while (0)

struct S { int m; int n; };
__attribute__((noinline)) int *addr (S *q, int i) { return &q[i].n; }

int main ()
{
  std::signal (SIGSEGV, on_segv);
  RUN (addr0, addr (nullptr, 0));
  RUN (addr1, addr (nullptr, 1));
  return 0;
}
// { dg-output "addr0 0(\n|\r\n|\r)addr1 0(\n|\r\n|\r)" }
