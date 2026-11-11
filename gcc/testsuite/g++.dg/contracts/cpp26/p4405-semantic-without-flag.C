// P4405: the prove semantic requested on the command line without
// -fcontracts-p4405 is not allowed; the compiler says so and the best fit,
// ignore, is used instead, so nothing is diagnosed and the predicate is not
// evaluated.
//
// Mirror: clang/test/Contracts/p4405-semantic-without-flag.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=prove" }
// { dg-warning "requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 }

static int evaluated;
bool chk (int x) { ++evaluated; return x > 0; }

int f (int x) pre (chk (x)) { return x; }

int main ()
{
  f (-1);
  return evaluated;
}
