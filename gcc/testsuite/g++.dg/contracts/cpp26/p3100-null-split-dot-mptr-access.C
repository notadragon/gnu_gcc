// P3100: the object expression of a `.*` access is checked for null like that
// of a class member access: (*q).*pm checks `q`, q[i].*pm the element's address
// `q + i` -- under the .access leaf of ub:expr.unary.dereference.nullptr where
// the member is read, under .indirection where only its address is formed.
// With only .access configured, a null `q` and a member at a nonzero offset,
// whose address q + pm is not null, are reported once in the read shapes, and
// not at all where only its address is formed.  A null dereference that is read
// faults after the handler returns; the fault is caught as SIGSEGV and resumed
// with siglongjmp, so one run counts the reports of every shape.  (q[i].*pm
// with i != 0 is not checked here: GCC checks `q + i`, Clang `q` -- divergence
// GCC-794.)
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-split-dot-mptr.cpp.
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
#define NI __attribute__((noinline))
NI int star_read (S *q, int S::*pm) { return (*q).*pm; }
NI int sub_read (S *q, int i, int S::*pm) { return q[i].*pm; }
NI int *star_addr (S *q, int S::*pm) { return &((*q).*pm); }
NI int *sub_addr (S *q, int i, int S::*pm) { return &(q[i].*pm); }

int main ()
{
  std::signal (SIGSEGV, on_segv);
  RUN (star_read, star_read (nullptr, &S::n));
  RUN (sub_read, sub_read (nullptr, 0, &S::n));
  RUN (star_addr, star_addr (nullptr, &S::n));
  RUN (sub_addr, sub_addr (nullptr, 0, &S::n));
  return 0;
}
// { dg-output "star_read 1(\n|\r\n|\r)sub_read 1(\n|\r\n|\r)star_addr 0(\n|\r\n|\r)sub_addr 0(\n|\r\n|\r)" }
