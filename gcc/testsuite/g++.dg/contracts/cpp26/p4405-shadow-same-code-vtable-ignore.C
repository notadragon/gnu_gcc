// p4405-shadow-same-code-vtable.C without a proof shadow (GCC-1000): every
// evaluation is ignore, so there is no obligation and no shadow, but each
// contract_assert still gets an assumption region, whose qualified call
// makes V::ok, an inline virtual function, and W::ok a definition here.
// The regions are removed from the real code before it is optimized, and
// the definitions are there for nothing real, so the real `v->ok (n)' and
// `w->ok (n)' do not fold from the external vtables to them: the object
// code is the same as without -fcontracts-p4405, where there are no
// regions.  V::ok is declared pure, so its predicate has no side effects;
// W::ok's has the call's (GCC-1003).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore" }

struct V { virtual int f (int); [[gnu::pure]] virtual bool ok (int n) const { return n > 0; } };
struct W { virtual int f (int); virtual bool ok (int n) const { return n > 0; } };

bool use (int n)
{
  V *v = new V;
  if (n > 1)
    contract_assert (v->V::ok (n));
  return v->ok (n);
}

bool use2 (int n)
{
  W *w = new W;
  if (n > 1)
    contract_assert (w->W::ok (n));
  return w->ok (n);
}

// { dg-final { contracts-same-object-code "-O1 -fno-contracts-p4405" "-O1" } }
// { dg-final { contracts-same-object-code "-Og -fno-contracts-p4405" "-Og" } }
// { dg-final { contracts-same-object-code "-O2 -fno-devirtualize -fno-contracts-p4405" "-O2 -fno-devirtualize" } }
// { dg-final { contracts-same-object-code "-O2 -fno-contracts-p4405" "-O2" } }
