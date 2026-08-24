// P3100: routing a null-dereference check must cover every syntactic shape
// of the same UB, not just a plain dereference.
//
// Regression test: every null/alignment gate in the P3100 work was given
// the "or a contract routes this" treatment except
// ubsan_maybe_instrument_reference_or_call, which hard-coded
// IMPLICIT_UB_NONE into the reaction operands.  That function is what
// instruments a reference binding (int &r = *p) and a null-this member
// call (p->f ()), so with a routed quick_enforce config a plain *p and a
// p->m got their trap guard while those two got no instrumentation at all
// -- same UB category, silently unenforced on two of four shapes, and no
// diagnostic to say so.
//
// The four call-site gates in cp-gimplify.cc had to be widened too, or
// the fixed function would never be reached without a sanitizer also
// enabled.
//
// Taking a dereference's address is deliberately NOT instrumented -- see
// p3100-null-deref-addr-of.C -- so the negative arm below guards that the
// fix did not overreach.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-null-ref-and-call.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct S
{
  int m;
  void f ();
};

/* Already worked: a plain dereference.  */
int
shape_load (int *p)
{
  return *p;
}

/* Regressed shape 1: binding a reference.  */
int &
shape_ref (int *p)
{
  int &r = *p;
  return r;
}

/* Regressed shape 2: a member call on a null object.  */
void
shape_call (S *p)
{
  p->f ();
}

/* Already worked: member access.  */
int
shape_mem (S *p)
{
  return p->m;
}

/* Must stay uninstrumented: &*p does not access the object.  */
int *
addr_of (int *p)
{
  return &*p;
}

// One guard for each of the four accessing shapes, and none for addr_of.
// { dg-final { scan-tree-dump-times "__builtin_trap" 4 "optimized" } }
