// GCC-3 / PR125459, PR125587: a contract condition that re-calls a
// constexpr function already called earlier in the same constant
// evaluation is wrongly rejected as non-constant.
//
// Extracted from the fix commit's own regression test,
// gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-repeat-call.C
// (added by gnu_gcc commit aeba77c0b85), found via
// `git show aeba77c0b85 --stat` and read with
// `git show aeba77c0b85:gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-repeat-call.C`.
// DejaGnu directive lines were stripped. The original test also exercises
// the P3400 assertion-label forms (`contract_assert<label_enforce{}>`,
// `contract_assert<label_observe{}>`), which require this branch's
// `-fcontracts-p3400` flag and a `<contracts>`-derived label type; those
// are dropped here since the commit message states the defect has
// "nothing to do with contract labels" and the unlabelled forms below
// exercise the same code path. Requires -fcontracts (C++26).

/* A contract condition that calls a constexpr function already called
   earlier in the same constant evaluation must still be constant.

   A contract condition is evaluated under a modifiable_tracker, so that a
   side-effecting predicate cannot modify objects belonging to the
   enclosing evaluation.  Membership of the "modifiable" set was decided by
   whether the object was already a key in the value map -- but
   destroy_value retires a VAR_/PARM_/RESULT_DECL by leaving it in that map
   mapped to void_node rather than removing it.  A constexpr function
   called once, returned from, and then called again from inside a
   contract condition therefore found its own RESULT_DECL already present,
   was refused permission to write it, and the condition came out
   non-constant:

     error: contract condition is not constant

   under enforce, and the same text as a warning under observe.

   Found by the BDE contracts integration, where a formatter's parse loop
   tests `spec.empty()` and then calls a helper asserting `!spec.empty()`.  */

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
  if (count<step_assert> ("xyx", 3) != 2)
    __builtin_abort ();
  if (count_nested ("xyx", 3) != 3)
    __builtin_abort ();
  return 0;
}
