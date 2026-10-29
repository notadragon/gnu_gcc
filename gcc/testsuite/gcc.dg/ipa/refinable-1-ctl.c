/* Without the mark, the facts derived from the same bodies are used: the
   control for refinable-1.c.  */
/* { dg-do compile } */
/* { dg-options "-O2 -std=gnu99 -fno-inline -fexceptions -fdump-tree-optimized" } */

#include "refinable-1.c"

/* { dg-final { scan-tree-dump-not "1001" "optimized" } } */
/* { dg-final { scan-tree-dump-not "1002" "optimized" } } */
/* { dg-final { scan-tree-dump-times "= c99_const \\\(" 1 "optimized" } } */
/* { dg-final { scan-tree-dump-not "resx|__builtin_eh_pointer|\\\[LP " "optimized" } } */
/* { dg-final { scan-tree-dump-not "2001" "optimized" } } */
