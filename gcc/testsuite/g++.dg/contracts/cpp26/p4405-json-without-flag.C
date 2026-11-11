// P4405: a contract configuration asking for prove or diagnose without
// -fcontracts-p4405 is warned about (-Wcontract-configuration), and ignore
// is used instead.
//
// Mirror: clang/test/Contracts/p4405-json-without-flag.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-json-without-flag.json" }
// { dg-warning "asks for .prove., which requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 }
// { dg-warning "asks for .diagnose., which requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 }

static int evaluated;
bool chk (int x) { ++evaluated; return x > 0; }

int f (int x) pre (chk (x)) post (r: chk (r)) { contract_assert (chk (x)); return x; }

int main ()
{
  f (-1);
  return evaluated;
}
