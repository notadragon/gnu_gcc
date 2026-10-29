// P3100: a call through a virtual pointer to member function reads the
// vtable pointer through the object pointer: with that pointer null it is
// reported once, under .access, as a virtual call is (DECISIONS.md E102
// (a)) -- also on the element at a constant subscript, `(vp[1].*p) ()',
// or at a constant offset, `((vp + 1)->*p) ()', and with a nonzero `this'
// adjustment (a virtual member function of a second polymorphic base).  A
// non-virtual pointer to member function reads nothing: the same calls
// are reported once under .indirection.  Through `this' in a member
// function called through a null pointer, nothing more is reported than
// the call (DECISIONS.md E106 (a)).  A non-null pointer is not reported.
// (GCC-837: the vtable pointer was read through the adjusted address or
// the element's, not null, and the virtual forms but the first were not
// reported.)  The forms are in p3100-null-virtual-pmf.h.
// With the whole group configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-virtual-pmf.h"
// { dg-output "virt 1(\n|\r\n|\r)virtsub 1(\n|\r\n|\r)virtarith 1(\n|\r\n|\r)" }
// { dg-output "virtadj 1(\n|\r\n|\r)virtadjsub 1(\n|\r\n|\r)nonvirt 1(\n|\r\n|\r)" }
// { dg-output "nonvirtsub 1(\n|\r\n|\r)nonvirtarith 1(\n|\r\n|\r)nonvirtadj 1(\n|\r\n|\r)" }
// { dg-output "nonvirtadjsub 1(\n|\r\n|\r)self 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
