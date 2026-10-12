/* Test a named-result postcondition whose condition mentions an unexpanded
   parameter pack.

   Regression test: rebuild_postconditions re-substitutes a postcondition
   once the return type is known, deliberately passing tsubst_expr an empty
   argument vector and relying on the local identity mappings it installs.
   But TMPL_ARGS_DEPTH reports depth 1 for a zero-length TREE_VEC -- only
   NULL_TREE reports 0 -- so tsubst_pack_expansion believed it had an
   argument level and read out of bounds of the empty vector, ICEing with
   "accessed elt N of 'tree_vec' with 0 elts" at the primary template's
   *declaration*.  No instantiation was needed to trigger it.

   The trigger is any mention of a pack, not fold-expressions
   specifically, and it needs a non-deduced return type (a deduced one
   makes rebuild_postconditions bail) and a named result (an unnamed one
   is skipped).  Note this is deliberately compiled with plain -fcontracts:
   it is a base-P2900 defect, not a P3850-extension one, and must keep
   working on the base feature.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations = 0;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

/* Binary fold over the pack.  */
template <class... T>
int fold_over_pack (const T... t)
  post (r : (int (t) + ... + 0) >= 0)
{ return 0; }

/* Unary fold.  */
template <class... T>
int unary_fold (const T... t)
  post (r : (int (t) + ...) >= 0)
{ return 0; }

/* No fold at all -- the pack is merely expanded into a call.  */
template <class... T>
int all_nonneg (T... t);

template <class... T>
int pack_in_call (const T... t)
  post (r : all_nonneg (t...) >= 0)
{ return 0; }

template <class... T>
int
all_nonneg (T... t)
{
  int lo = 0;
  ((lo = t < lo ? t : lo), ...);
  return lo;
}

/* sizeof... only: no value pack in the condition at all.  */
template <class... T>
int count_pack (const T... t)
  post (r : sizeof... (t) >= 0)
{ return 0; }

/* A type pack rather than a value pack.  */
template <class... T>
int type_pack (const T... t)
  post (r : (sizeof (T) + ... + 0) > 0)
{ return 0; }

/* A non-type parameter pack.  */
template <int... N>
int nttp_pack ()
  post (r : (N + ... + 0) >= 0)
{ return 0; }

/* A declaration only, with the definition supplied later.  */
template <class... T>
int declared_first (const T... t)
  post (r : (int (t) + ... + 0) >= 0);

template <class... T>
int
declared_first (const T... t)
{ return 0; }

/* A member of a class template, defined in class.  */
template <class U>
struct Holder
{
  template <class... T>
  int f (const T... t)
    post (r : (int (t) + ... + 0) >= 0)
  { return 0; }
};

/* A trailing return type.  */
template <class... T>
auto trailing (const T... t) -> int
  post (r : (int (t) + ... + 0) >= 0)
{ return 0; }

int
main ()
{
  /* Satisfied: no violation.  */
  violations = 0;
  fold_over_pack (1, 2, 3);
  unary_fold (1, 2);
  pack_in_call (1, 2);
  count_pack (1, 2);
  type_pack (1, 2);
  nttp_pack<1, 2> ();
  declared_first (1, 2);
  Holder<int> ().f (1, 2);
  trailing (1, 2);
  if (violations != 0)
    __builtin_abort ();

  /* Violated: the postcondition must actually be checked, not merely
     have compiled.  */
  violations = 0;
  fold_over_pack (-1, -2);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  pack_in_call (-1, 2);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  nttp_pack<-5> ();
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  Holder<int> ().f (-3);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  trailing (-4);
  if (violations != 1)
    __builtin_abort ();

  return 0;
}
