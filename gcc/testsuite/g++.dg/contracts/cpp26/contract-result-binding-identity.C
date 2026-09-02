// [dcl.contract.res]/1 binds a postcondition's result name to one object.  A
// scalar result is a gimple register, so taking its address used to spill it
// to a fresh temporary EVERY time -- and a predicate may take it more than
// once, which gave two addresses for one result binding inside a single
// evaluation of a single predicate (PR112794).
//
// [dcl.contract.res] Example 2 permits the implementation to introduce a
// temporary for a register-returned result, so which object the binding names
// is unspecified for such a type.  It does not permit two, and it does not
// permit the predicate's value to come out wrong: [basic.contract.eval]
// defines B as "the value that would result from evaluating the predicate"
// and a violation occurs when B is false.
//
// The value-based case below is the sharp one -- `same (r, &r)' is plainly
// true, so no violation may occur; GCC used to report one and abort.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }

const int *a1 = nullptr;
const int *a2 = nullptr;

const int *addr_via_ref (const int &i) { return &i; }

bool rec1 (const int *p) { a1 = p; return true; }
bool rec2 (const int *p) { a2 = p; return true; }

/* Same binding, two ways of taking its address, one predicate.  */
int
two_addresses () post (r : rec1 (&r) && rec2 (addr_via_ref (r)))
{
  return 7;
}

/* The same disagreement expressed as the predicate's value.  */
bool same (const int &a, const int *b) { return &a == b; }

int
value_form () post (r : same (r, &r))
{
  return 7;
}

/* A mutation through the binding must still reach the caller: having
   evaluated the predicate we owe its effects.  expr.prim.id.unqual.p7-4.C
   pins this too; it is repeated here because the fix for the above is what
   could most easily break it.  */
int
mutation_observed () post (r : (const_cast<int &> (r)++, true))
{
  return 1;
}

/* A class returned in memory: Example 2 requires &r to be the returned
   object itself, so no stand-in may be introduced for this one.  */
struct Big
{
  Big () {}
  Big (const Big &) {}
  int x = 0;
  const Big *self = nullptr;
};

Big
memory_returned (Big *const ptr) post (r : &r == ptr)
{
  return {};
}

int
main ()
{
  two_addresses ();
  if (a1 != a2)
    __builtin_abort ();

  value_form ();

  if (mutation_observed () != 2)
    __builtin_abort ();

  Big b = memory_returned (&b);
  (void) b;

  return 0;
}
