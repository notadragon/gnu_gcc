// A contract predicate we actually evaluate must be evaluated faithfully:
// its effects are observable, and that must not depend on whether the checks
// are outlined.  [basic.contract.eval] permits eliding a predicate -- an
// alternative evaluation "that produces the same value ... but has no side
// effects" -- which is all-or-nothing.  It does not permit evaluating one and
// then dropping some of its effects, which is what passing a by-value
// parameter (or the result binding) to an outlined check by value did:
// mutations of shared state still propagated while mutations of the copied
// entity vanished.
//
// partial_application below is the sharp case: faithful gives 303, a fully
// elided predicate gives 200, and the old outlined code gave 203 -- a state
// neither reading can produce.
//
// Run both ways; the results must agree.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-checks-outlined" }

int g = 0;

// (1) By-value parameter: the body must see the precondition's increment.
//     This is the unambiguous half.
int
by_value_parm (int n) pre (const_cast<int &> (n)++)
{
  return n;                     // required: 3.  outlined gives 2.
}

// (2) Result binding: same shape, but see PR112794 before "fixing" it.
int
result_binding () post (r : const_cast<int &> (r)++)
{
  return 1;                     // caller sees 2 inlined, 1 outlined.
}

// (3) Control: a reference parameter survives, because the reference is what
//     is copied.
int
ref_parm (int &n) pre (const_cast<int &> (n)++)
{
  return n;
}

// (4) Control: a global survives, being no one's parameter.
void
global_mut () pre (const_cast<int &> (g)++)
{
}

int g_log = 0;

bool
bump (int &n)
{
  ++n;
  g_log = n;                    // shared state: propagates even when outlined
  return true;
}

// One predicate, two side effects -- one on a by-value parameter, one on a
// global.  Either both happen or neither may; outlining does exactly one.
int
partial_application (int n) pre (bump (const_cast<int &> (n)))
{
  return n * 100 + g_log;       // faithful 303, elided 200, outlined 203
}

int
main ()
{
  int bad = 0;

  {
    int r = partial_application (2);
    if (r != 303 && r != 200)
      bad |= 16;                // 203: neither faithful nor elided
  }

  if (by_value_parm (2) != 3)
    bad |= 1;                   // fires only when outlined

  if (result_binding () != 2)
    bad |= 2;                   // fires only when outlined

  int v = 2;
  ref_parm (v);
  if (v != 3)
    bad |= 4;                   // never fires

  g = 3;
  global_mut ();
  if (g != 4)
    bad |= 8;                   // never fires

  return bad;                   // inlined: 0.  outlined: 19 (16|2|1).
}
