// P4405: -fcontract-group-evaluation-semantic= asking for prove or diagnose
// without -fcontracts-p4405 is warned about (-Wcontract-configuration), and
// ignore is used instead.
//
// Mirror: clang/test/Contracts/p4405-group-without-flag.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-group-evaluation-semantic=g:prove,h:diagnose" }
// { dg-warning "asks for .prove., which requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 }
// { dg-warning "asks for .diagnose., which requires .-fcontracts-p4405.; .ignore. is used instead" "" { target *-*-* } 0 }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int evaluated;
bool chk (int x) { ++evaluated; return x > 0; }

int f (int x) pre<"g"group> (chk (x)) { return x; }
int g (int x) pre<"h"group> (chk (x)) { return x; }

int main ()
{
  f (-1);
  g (-1);
  return evaluated;
}
