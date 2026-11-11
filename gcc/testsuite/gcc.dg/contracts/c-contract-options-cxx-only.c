/* DECISIONS.md E179 (3) (GCC-970): a C++-only contracts per-paper option
   on a C input says it is valid for C++ but not for C, and does not turn C
   contracts on.  The negative forms are
   c-contract-options-cxx-only-negative.c; beside a C per-paper option,
   c-contract-options-cxx-only-mixed.c.

   Mirror: clang/test/Contracts/C/c-contract-options-cxx-only.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p3290 -fcontracts-p3400 -fcontracts-p4283 -fcontracts-p4301 -fcontracts-p4451" } */
/* { dg-warning ".-fcontracts-p3097. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p3098. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p3099. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p3290. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p3400. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p4283. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p4301. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */
/* { dg-warning ".-fcontracts-p4451. is valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#ifdef __gcc_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
