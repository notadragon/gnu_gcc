// P3100: a call through a null pointer of a function of a virtual base,
// virtual or not, converts the pointer to the virtual base, reading the
// base's offset through it: the .access leaf of
// {expr.unary.dereference.nullptr} (DECISIONS.md E94 (b)), reported once.
// For a virtual call the conversion's check of the pointer is made before
// the object operand and the vtable lookup, which read the offset too,
// are evaluated, and their reads are not checked again (GCC-859: a
// virtual call was reported twice at -O1 and above).  The forms are in
// p3100-null-virtual-call-vbase.h.
// At -O0.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-virtual-call-vbase.h"

// { dg-output "vbh 1(\n|\r\n|\r)vbstar 1(\n|\r\n|\r)vbsub 1(\n|\r\n|\r)" }
// { dg-output "vbq 1(\n|\r\n|\r)vbnv 1(\n|\r\n|\r)vbmem 1(\n|\r\n|\r)" }
