// P3100: a call through a pointer to member function of a base class on
// the object a null pointer points to, converted to the base first --
// `(mp->*p) ()', `((*mp).*p) ()' with p a `void (W2::*) ()' and W2 a base
// of mp's class, at offset zero (MW's primary base, DW's only base) or not
// (MV's second base), on an element at a constant subscript or offset, on
// a member subobject -- takes the leaf of its P3100 null check from the
// pointer's virtual bit (DECISIONS.md E102 (a)), as a call through a
// pointer to member function of the class itself does: a virtual one,
// which reads the vtable pointer through the object pointer, is reported
// once under .access, a non-virtual one once under .indirection.  A
// non-null pointer is not reported.  (GCC-864: the object pointer was
// checked by the ubsan pass where the base's address is formed, under
// .indirection, and the vtable pointer's read on the adjusted address: a
// virtual one was reported under both leaves at offset zero, under
// .indirection only at a nonzero offset, and not at all on an element at
// a constant subscript.)  The forms are in p3100-null-pmf-base.h; Clang's
// Runnable/p3100-null-pmf-base.cpp mirrors them, and its
// `((*(mp + 1)).*p) ()' (CLANG-800) is mirrored here.
// With the whole group configured, at -O0.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O0" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-pmf-base.h"

// { dg-output "mwpmf 1(\n|\r\n|\r)mwpmfv 1(\n|\r\n|\r)mwdpmf 1(\n|\r\n|\r)" }
// { dg-output "mwdpmfv 1(\n|\r\n|\r)mvpmf 1(\n|\r\n|\r)mvpmfv 1(\n|\r\n|\r)" }
// { dg-output "mvdpmf 1(\n|\r\n|\r)mvdpmfv 1(\n|\r\n|\r)dwpmf 1(\n|\r\n|\r)" }
// { dg-output "dwpmfv 1(\n|\r\n|\r)mwsub 1(\n|\r\n|\r)mwsubv 1(\n|\r\n|\r)" }
// { dg-output "mvsub 1(\n|\r\n|\r)mvsubv 1(\n|\r\n|\r)mwarith 1(\n|\r\n|\r)" }
// { dg-output "mwarithv 1(\n|\r\n|\r)mwdarith 1(\n|\r\n|\r)" }
// { dg-output "mwdarithv 1(\n|\r\n|\r)hwmem 1(\n|\r\n|\r)hwmemv 1(\n|\r\n|\r)" }
// { dg-output "hwmemptr 1(\n|\r\n|\r)hwmemptrv 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
