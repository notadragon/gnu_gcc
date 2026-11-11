/* DECISIONS.md E179 (1) (GCC-970): -fcontracts-p3850 works on a C input.
   It turns on the C per-paper options (-fcontracts-p3100,
   -fcontracts-p4298 and -fcontracts-p4405), and so -fcontracts-p4299, as
   it turns -fcontracts on in C++.  Which options it implies is
   c-contract-options-p3850-implied*.c.

   Mirror: clang/test/Contracts/C/c-contract-options-p3850-implies-p4299.c
   in the llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3850" } */
/* { dg-bogus "valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifndef __gcc_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f (int x) _Pre (x > 0) { return x; }
