// P4405: likewise for the diagnose semantic: without -fcontracts-p4405 it
// is not allowed, the compiler says so, and ignore is used instead.
//
// Mirror: clang/test/Contracts/p4405-diagnose-without-flag.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=diagnose" }
// { dg-warning "requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 }

static int evaluated;
bool chk (int x) { ++evaluated; return x > 0; }

int f (int x) pre (chk (x)) post (r: chk (r)) { contract_assert (chk (x)); return x; }

int main ()
{
  f (-1);
  return evaluated;
}
