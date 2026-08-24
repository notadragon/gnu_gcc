/* A contract condition that calls a constexpr function already called
   earlier in the same constant evaluation must still be constant.

   Regression test.  A contract condition is evaluated under a
   modifiable_tracker, so that a side-effecting predicate cannot modify
   objects belonging to the enclosing evaluation.  Membership of the
   "modifiable" set was decided by whether the object was already a key in
   the value map -- but destroy_value retires a VAR_/PARM_/RESULT_DECL by
   leaving it in that map mapped to void_node rather than removing it.  A
   constexpr function called once, returned from, and then called again from
   inside a contract condition therefore found its own RESULT_DECL already
   present, was refused permission to write it, and the condition came out
   non-constant:

     error: contract condition is not constant

   under enforce, and the same text as a warning under observe.  Nothing to
   do with labels; the labelled case below is only the shape this was
   reported from.

   Found by the BDE contracts integration, where a formatter's parse loop
   tests `spec.empty()` and then calls a helper asserting `!spec.empty()`.

   A predicate that really does modify the enclosing evaluation is a
   separate matter, covered by contract-constexpr-side-effect.C.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;

struct label_enforce
{
  using assertion_control_object = label_enforce;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::enforce; }
};

struct label_observe
{
  using assertion_control_object = label_observe;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::observe; }
};

struct view
{
  const char *p;
  unsigned len;
  constexpr bool empty () const { return len == 0; }
  constexpr char front () const { return *p; }
  constexpr void advance () { ++p; --len; }
};

/* contract_assert: the caller's loop calls empty(), then this calls it
   again from the condition.  */
constexpr unsigned
step_assert (view *v)
{
  contract_assert (!v->empty ());
  unsigned c = v->front () == 'x' ? 1u : 0u;
  v->advance ();
  return c;
}

/* Same, via a precondition.  */
constexpr unsigned
step_pre (view *v) pre (!v->empty ())
{
  unsigned c = v->front () == 'x' ? 1u : 0u;
  v->advance ();
  return c;
}

/* Same, via a postcondition -- whose condition also calls empty(), and runs
   after the body has called it too.  */
constexpr unsigned
step_post (view *const v) post (r : r <= 1u && (v->empty () || !v->empty ()))
{
  unsigned c = v->front () == 'x' ? 1u : 0u;
  v->advance ();
  return c;
}

/* Labelled forms: enforce and observe.  The observe one used to produce a
   spurious warning rather than an error, so it needs checking too.  */
constexpr unsigned
step_labelled (view *v)
{
  contract_assert<label_enforce{}> (!v->empty ());
  unsigned c = v->front () == 'x' ? 1u : 0u;
  v->advance ();
  return c;
}

constexpr unsigned
step_labelled_observe (view *v)
{
  contract_assert<label_observe{}> (!v->empty ());
  unsigned c = v->front () == 'x' ? 1u : 0u;
  v->advance ();
  return c;
}

/* The driver: empty() is called in the loop condition, so by the time the
   contract's condition calls it the RESULT_DECL is in the map already.  */
template <unsigned (*STEP) (view *)>
constexpr unsigned
count (const char *s, unsigned n)
{
  view v { s, n };
  unsigned total = 0;
  while (!v.empty ())
    total += STEP (&v);
  return total;
}

static_assert (count<step_assert> ("xyx", 3) == 2);
static_assert (count<step_pre> ("xyx", 3) == 2);
static_assert (count<step_post> ("xyx", 3) == 2);
static_assert (count<step_labelled> ("xyx", 3) == 2);
static_assert (count<step_labelled_observe> ("xyx", 3) == 2);

/* A nested call one level deeper, so the reused RESULT_DECL belongs to a
   frame further out than the condition's own caller.  */
constexpr bool
outer_empty (const view *v) { return v->empty (); }

constexpr unsigned
step_nested (view *v)
{
  contract_assert (!outer_empty (v));
  v->advance ();
  return 1;
}

constexpr unsigned
count_nested (const char *s, unsigned n)
{
  view v { s, n };
  unsigned total = 0;
  while (!outer_empty (&v))
    total += step_nested (&v);
  return total;
}

static_assert (count_nested ("xyx", 3) == 3);

int
main ()
{
  /* Run-time evaluation of the same functions, to confirm the fix did not
     disturb ordinary code generation.  */
  if (count<step_assert> ("xyx", 3) != 2)
    __builtin_abort ();
  if (count<step_labelled> ("xyx", 3) != 2)
    __builtin_abort ();
  if (count_nested ("xyx", 3) != 3)
    __builtin_abort ();
  return 0;
}
