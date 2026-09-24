/* pre, post and contract_assert whose predicate re-calls a constexpr
   function already evaluated in the same constant evaluation.

   Companion to contract-constexpr-repeat-call.C, which covers the
   pointer-to-a-local shape this was first reported from (GCC-3, PR125459).
   This file covers the shapes the 2026-09-21 follow-up on that PR raised,
   none of which that file reaches:

     * a temporary materialised INSIDE the predicate, against one made in the
       body or the caller and made again inside the predicate;

     * functions returning a class by value, so the callee has a RESULT_DECL
       with real storage -- the value-map entry the original defect was
       actually about -- including a result-name binding on `post', and one
       passed to a function taking a reference;

     * all three assertion kinds, which differ in WHERE the earlier call can
       come from.  `pre' runs before the body, so only the caller can have
       made it.  `contract_assert' can be preceded by a call in the same
       body.  `post' runs after the body, so a body call always precedes it.
       That last one is the point of the follow-up: the workaround offered
       for the original report was to reorder the two statements, and a
       postcondition cannot be reordered.

   THE CONSTEXPR CALL CACHE IS THE TRAP IN THIS FILE.  A repeat call with
   identical arguments to a cacheable function is served from the cache
   without re-evaluating the body, so the shape under test never occurs and
   the case passes for the wrong reason.  Two constructs below defeat it:

     * a reference parameter bound to a prvalue -- a fresh temporary every
       call, so the call key never repeats;
     * a type with a non-trivial destructor (Guard, BoxG), which makes the
       call uncacheable, so identical arguments still re-evaluate.

   Do not give Guard or BoxG a trivial destructor: that makes their cases
   stop testing anything while still passing.

   WHICH CASES ACTUALLY REACH THE DEFECT, measured against stock trunk
   17.0.0 20260909, which does not carry the fix: nine of them, marked
   `[regresses]' below.  The rest are controls -- shapes that must keep
   compiling, and which a change in this area could plausibly break.  Two
   results are worth keeping in view, because both are the opposite of what
   the obvious guess predicts:

     * A by-value parameter does NOT reach it, with an identical argument or
       a differing one, whether the parameter is a scalar, a small class or
       a large one, and whether or not the callee returns a class into the
       result slot.  Six such shapes were probed and none regressed.  The
       differing-argument cases here are therefore controls, not
       cache-defeating instruments.

     * For the return-value slot, only the `post' forms reach it.  The same
       re-call written as a contract_assert in the body (box_assert,
       boxg_assert) does not.  That matches the follow-up's own point:
       `post' is the kind whose predicate cannot be moved before the body,
       and it is also the kind that fails here.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct Tag
{
  int v;
  constexpr bool ok () const { return v >= 0; }
};

/* Reference parameter: a prvalue argument materialises a new temporary for
   every call, so two calls never share a cache key.  */
constexpr bool by_ref (const Tag &t) { return t.v >= 0; }

/* By value, and so cacheable: every use below passes a different argument
   the second time.  */
constexpr bool by_val (Tag t) { return t.v >= 0; }

/* A non-trivial destructor makes the call uncacheable, so a repeat with
   identical arguments still re-evaluates the body.  */
struct Guard
{
  int v;
  constexpr Guard (int x) : v (x) {}
  constexpr ~Guard () {}
};
constexpr bool by_guard (const Guard &g) { return g.v >= 0; }

/* Returned by value, so the callee has a RESULT_DECL with storage.  Box is
   cacheable; BoxG is not.  */
struct Box
{
  int a, b, c;
  constexpr bool ok () const { return a >= 0; }
};
constexpr Box make_box (int a) { Box b { a, a + 1, a + 2 }; return b; }

struct BoxG
{
  int a, b, c;
  constexpr BoxG (int x) : a (x), b (x + 1), c (x + 2) {}
  constexpr ~BoxG () {}
  constexpr bool ok () const { return a >= 0; }
};
constexpr BoxG make_boxg (int a) { BoxG b (a); return b; }

constexpr bool box_ok_ref (const Box &b) { return b.a >= 0; }

/* ---- contract_assert: the earlier call sits above it in the body ------- */

constexpr int
ca_temp_inside (int s)
{
  bool first = by_ref (Tag { s });
  contract_assert (by_ref (Tag { s }));		/* [regresses] */
  return first ? s : -1;
}

constexpr int
ca_temp_outside (int s)
{
  Tag t { s };			/* made out here, not in the predicate */
  bool first = by_val (t);
  contract_assert (by_val (Tag { s + 1 }));	/* control */
  return first ? s : -1;
}

constexpr int
ca_guard (int s)
{
  bool first = by_guard (Guard (s));
  contract_assert (by_guard (Guard (s)));	/* [regresses] */
  return first ? s : -1;
}

static_assert (ca_temp_inside (1) == 1);
static_assert (ca_temp_outside (1) == 1);
static_assert (ca_guard (1) == 1);

/* ---- pre: only the caller can have made the earlier call --------------- */

/* [regresses] */
constexpr int pre_temp_inside (int s) pre (by_ref (Tag { s })) { return s; }
/* controls */
constexpr int pre_temp_outside (int s) pre (by_val (Tag { s + 1 }))
{ return s; }
constexpr int pre_guard (int s) pre (by_guard (Guard (s))) { return s; }

constexpr int
drive_pre (int s)
{
  bool a = by_ref (Tag { s });
  bool b = by_val (Tag { s + 1 });
  bool c = by_guard (Guard (s));
  return (a && b && c)
	 ? pre_temp_inside (s) + pre_temp_outside (s) + pre_guard (s)
	 : -1;
}

static_assert (drive_pre (1) == 3);

/* ---- post: the body always precedes the predicate ---------------------- */
/* A by-value parameter odr-used in a postcondition has to be const.  */

constexpr int
/* [regresses] */
post_temp_inside (const int s) post (by_ref (Tag { s }))
{
  bool first = by_ref (Tag { s });
  return first ? s : -1;
}

constexpr int
/* control */
post_temp_outside (const int s) post (by_val (Tag { s + 1 }))
{
  Tag t { s };
  bool first = by_val (t);
  return first ? s : -1;
}

constexpr int
post_guard (const int s) post (by_guard (Guard (s)))	/* [regresses] */
{
  bool first = by_guard (Guard (s));
  return first ? s : -1;
}

static_assert (post_temp_inside (1) == 1);
static_assert (post_temp_outside (1) == 1);
static_assert (post_guard (1) == 1);

/* ---- the return-value slot --------------------------------------------- */

/* control: the same re-call as a contract_assert does NOT regress */
constexpr Box
box_assert (int s)
{
  Box first = make_box (s);
  contract_assert (make_box (s + 1).ok ());
  return first;
}

constexpr BoxG
boxg_assert (int s)
{
  BoxG first = make_boxg (s);
  contract_assert (make_boxg (s).ok ());
  return first;
}

/* A result name binds the return slot itself, and these predicates both read
   it and re-call the function that filled it.  */

constexpr Box
box_post_named (const int s) post (r : r.ok () && make_box (s + 1).ok ())
/* [regresses] */
{
  return make_box (s);
}

constexpr BoxG
boxg_post_named (const int s) post (r : r.ok () && make_boxg (s).ok ())
/* [regresses] */
{
  return make_boxg (s);
}

/* The result binding passed to a function taking a reference, alongside a
   fresh temporary through the same parameter.  */

constexpr Box
box_post_by_ref (const int s)
  /* [regresses] */
  post (r : box_ok_ref (r) && box_ok_ref (make_box (s + 1)))
{
  return make_box (s);
}

static_assert (box_assert (1).a == 1);
static_assert (boxg_assert (1).a == 1);
static_assert (box_post_named (1).a == 1);
static_assert (boxg_post_named (1).a == 1);
static_assert (box_post_by_ref (1).a == 1);

/* ---- a pre and a post on one function, both re-calling ----------------- */

constexpr Box
both_ends (const int s)
  pre (by_ref (Tag { s }))
  post (r : r.ok () && by_ref (Tag { s }))		/* [regresses] */
{
  Box b = make_box (s);
  contract_assert (by_ref (Tag { s }));
  return b;
}

static_assert (both_ends (1).a == 1);

int
main ()
{
  /* The same functions at run time, so the constant-evaluation fix is not
     resting on code that only ever folds.  */
  if (ca_temp_inside (1) != 1 || ca_temp_outside (1) != 1 || ca_guard (1) != 1)
    __builtin_abort ();
  if (drive_pre (1) != 3)
    __builtin_abort ();
  if (post_temp_inside (1) != 1 || post_temp_outside (1) != 1
      || post_guard (1) != 1)
    __builtin_abort ();
  if (box_assert (1).a != 1 || boxg_assert (1).a != 1)
    __builtin_abort ();
  if (box_post_named (1).a != 1 || boxg_post_named (1).a != 1
      || box_post_by_ref (1).a != 1)
    __builtin_abort ();
  if (both_ends (1).a != 1)
    __builtin_abort ();
  return 0;
}
