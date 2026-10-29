// P3100: a dynamic_cast to a reference type of the object a null pointer
// points to (`dynamic_cast<W &> (*vp)', to a const reference, of an element
// `vp[1]', of a member `hp->m') reads the object's vtable pointer through
// the pointer, as a virtual call does: reported once, under .access
// (DECISIONS.md E102 (a); it was .indirection, checked as a non-virtual
// member function called on the object is).  One of a reference parameter
// is not checked again (the null reference is reported once, under
// .indirection, where it is bound), nor one of an object.  Then Clang's:
// one at an offset, `*(vp + 1)', and of `*xp' converted to a polymorphic
// base at a nonzero offset, each once under .access.  The forms are in
// p3100-null-dynamic-cast-reference.h.
// Mirror: llvm_llvm-project
// clang/test/Contracts/Runnable/p3100-null-dynamic-cast-reference.cpp.
// With .indirection configured, at -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-null-dynamic-cast-reference.h"
// { dg-output "star 0(\n|\r\n|\r)cstar 0(\n|\r\n|\r)sub 0(\n|\r\n|\r)" }
// { dg-output "member 0(\n|\r\n|\r)refparam 1(\n|\r\n|\r)nonnull 0(\n|\r\n|\r)" }
// { dg-output "arith 0(\n|\r\n|\r)base 0(\n|\r\n|\r)nonnull_clang 0(\n|\r\n|\r)" }
