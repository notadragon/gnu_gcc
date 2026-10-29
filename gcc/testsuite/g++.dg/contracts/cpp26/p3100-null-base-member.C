// P3100: a member of a base class used through a null pointer is one null
// dereference, reported once under an observe semantic and under the leaf
// of the use, as a member of the class itself is.  The object pointer is
// checked before its conversion to the base; for a virtual base that
// conversion loads the base's offset through the object's vptr -- an access
// of the object -- so every use of a virtual base's member is the .access
// leaf, and a base at a nonzero offset (whose converted address is not
// null) is checked on the object pointer too.  The same holds when the
// object expression is a member subobject (p->d.b, checked once, on p), a
// subscript (q[i].b with i == 0), a pointer loaded from memory ((*pp)->b),
// the object of a .* (((*p).*pmf) (), (*p).*pm), and for (p->*pmf) () and
// &(p->*pm) with a virtual base.  A base at offset 0 (rd_first) is the
// control.  The forms are in p3100-null-base-member.h.
// With the whole group configured, every form is reported once.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-base-member.cpp.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-base-member.h"
// { dg-output "v_rd 1(\n|\r\n|\r)v_wr 1(\n|\r\n|\r)v_addr 1(\n|\r\n|\r)" }
// { dg-output "v_ref 1(\n|\r\n|\r)v_call 1(\n|\r\n|\r)v_rd_star 1(\n|\r\n|\r)" }
// { dg-output "v_call_star 1(\n|\r\n|\r)v_bind 1(\n|\r\n|\r)v_cast 1(\n|\r\n|\r)" }
// { dg-output "v_member 1(\n|\r\n|\r)v_sub 1(\n|\r\n|\r)n_rd 1(\n|\r\n|\r)" }
// { dg-output "n_addr 1(\n|\r\n|\r)n_ref 1(\n|\r\n|\r)n_call 1(\n|\r\n|\r)" }
// { dg-output "n_rd_star 1(\n|\r\n|\r)n_call_star 1(\n|\r\n|\r)n_bind 1(\n|\r\n|\r)" }
// { dg-output "n_loaded 1(\n|\r\n|\r)n_sub 1(\n|\r\n|\r)m_rd 1(\n|\r\n|\r)" }
// { dg-output "m_call 1(\n|\r\n|\r)m_ref 1(\n|\r\n|\r)m_rd_off 1(\n|\r\n|\r)" }
// { dg-output "n_pmf_arrow 1(\n|\r\n|\r)n_pmf_dot 1(\n|\r\n|\r)n_pm_dot 1(\n|\r\n|\r)" }
// { dg-output "v_pmf_arrow 1(\n|\r\n|\r)v_pmf_dot 1(\n|\r\n|\r)v_pm_dot 1(\n|\r\n|\r)" }
// { dg-output "v_pm_addr 1(\n|\r\n|\r)rd_first 1(\n|\r\n|\r)" }
