/* Without -fcontracts-p4299 the contract configuration options are not used,
   so they are not parsed either: say so rather than accept them silently.
   Clang warns that the argument is unused.

   Mirror (C++, contracts off): g++.dg/contracts/cpp26/contracts-config-without-contracts.C  */
/* { dg-do compile } */
/* { dg-options "-fcontract-group-evaluation-semantic=a:bogus" } */
/* { dg-warning "contract configuration options have no effect without .-fcontracts-p4299." "" { target *-*-* } 0 } */

int x;
