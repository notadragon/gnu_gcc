// Without the mark, the facts derived from the external body are used:
// the control for refinable-extern.C.
// { dg-do compile }
// { dg-options "-O2 -fno-inline -fdump-tree-optimized" }

#include "refinable-extern.C"

// { dg-final { scan-tree-dump-not "1001" "optimized" } }
