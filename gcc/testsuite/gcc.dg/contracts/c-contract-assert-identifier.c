/* In C, `contract_assert' is an ordinary identifier -- P4299 spells the
   keyword _ContractAssert, and <stdcontracts.h> maps the lowercase name to it
   with a macro -- so a function of that name compiles.

   Mirror: clang/test/Contracts/c-contract-assert-identifier.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4299 -pedantic-errors" } */

int contract_assert (int);

void f (int x) { contract_assert (x > 0); }
