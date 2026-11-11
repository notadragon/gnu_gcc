/* -fcontracts on a C input is a C++-only option, and draws a warning.

   Mirror: clang/test/Contracts/c-fcontracts-warning.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts" } */
/* { dg-warning "valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

int f (int x) { return x; }
