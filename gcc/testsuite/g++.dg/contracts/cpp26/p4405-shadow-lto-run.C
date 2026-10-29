// P4405 stage 3: a program compiled and linked with -flto, its proof
// shadows diagnosed at the compile step, links and runs; nothing of a
// shadow reaches the link-time compilation, and the predicates never run.
//
// No Clang mirror: the llvm_llvm-project fork's tests have no LTO linker.
// { dg-do run { target c++26 } }
// { dg-require-effective-target lto }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -flto" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose" }

static int evaluated;
[[gnu::noinline]] bool touch (bool v) { ++evaluated; return v; }

[[gnu::noinline]] int f (int i, int n)
{
  if (i < n)
    contract_assert (i < n);	// { dg-bogus "cannot be proven|always false" }
  contract_assert (touch (i < n));	// { dg-warning "contract assertion cannot be proven" }
  return i;
}

int main ()
{
  if (f (1, 2) + f (3, 2) != 4 || evaluated != 0)
    __builtin_abort ();
}
