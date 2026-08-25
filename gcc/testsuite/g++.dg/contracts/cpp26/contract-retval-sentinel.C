/* A contract on a function returning a class with a non-trivial destructor
   ICEd in the gimplifier:

     internal compiler error: in gimple_add_tmp_var, at gimplify.cc:845

   maybe_apply_function_contracts runs from finish_function with the
   sk_function_parms level current, and wraps the finished body in an
   artificial block.  do_poplevel pops that block's own level before calling
   maybe_splice_retval_cleanup, so the latter sees sk_function_parms again --
   exactly the test it uses to recognise the function body.  The real body's
   closing brace had already passed that same test, so current_retval_sentinel
   got a second DECL_EXPR (which the gimplifier rejects), and, where the
   function also has a throwing cleanup, a second retval CLEANUP_STMT that
   would destroy the return value twice on throw.

   The sentinel is only created for a return type with a non-trivial
   destructor, and then only for a named return value or a throwing cleanup,
   which is why an ordinary contract on an ordinary function never tripped it.
   Codegen is required: -fsyntax-only alone compiled clean.

   Reproduces with plain -fcontracts; no P3850/P3097/P3100/P4298 extension is
   involved.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int live = 0;

struct Counted {
  int v;
  Counted () : v (0) { ++live; }
  Counted (int x) : v (x) { ++live; }
  Counted (const Counted &o) : v (o.v) { ++live; }
  ~Counted () { --live; }		/* non-trivial: required */
  Counted &operator++ () { ++v; return *this; }
  bool operator< (const Counted &o) const { return v < o.v; }
};

/* The named return value is returned from two loops that exit by different
   means: one via `break', falling through to a `return' after the loop, the
   other via a `return' written inside the loop body.  Both are needed to
   make the return value an NRV candidate here.  */
#define NRV_BODY(LIMIT)					\
  Counted result (start);				\
  ++result;						\
  if (flag)						\
    {							\
      while (1)						\
	{						\
	  --n;						\
	  if (0 == n)					\
	    break;					\
	  ++result;					\
	}						\
      return result;					\
    }							\
  Counted limit (LIMIT);				\
  while (result < limit)				\
    {							\
      --n;						\
      if (0 == n)					\
	return result;					\
      ++result;						\
    }							\
  return result;

struct S {
  bool flag;

  /* (1) precondition -- the shape that ICEd.  */
  Counted pre_only (int start, int n) const pre (n >= 1);

  /* (2) postcondition only.  */
  Counted post_only (int start, int n) const post (r : r.v >= 0);

  /* (3) both.  */
  Counted pre_and_post (int start, int n) const
    pre (n >= 1) post (r : r.v >= 0);

  /* (4) control: the same body with no contract at all.  */
  Counted no_contract (int start, int n) const;
};

Counted S::pre_only (int start, int n) const { NRV_BODY (100) }
Counted S::post_only (int start, int n) const { NRV_BODY (100) }
Counted S::pre_and_post (int start, int n) const { NRV_BODY (100) }
Counted S::no_contract (int start, int n) const { NRV_BODY (100) }

/* A free function, to show the member-ness is not what matters.  */
Counted free_pre (int start, int n, bool flag) pre (n >= 1);
Counted free_pre (int start, int n, bool flag) { NRV_BODY (100) }

/* The other way to get a sentinel: a cleanup that might throw.  The return
   object is constructed, then the local's destructor throws on the way out,
   so the retval cleanup is the thing that must destroy it -- exactly once.  */
struct ThrowOnDestroy {
  bool armed;
  ~ThrowOnDestroy () noexcept (false) { if (armed) throw 42; }
};

Counted throwing_cleanup_pre (bool arm) pre (true)
{
  ThrowOnDestroy guard { arm };
  Counted result (7);
  return result;
}

Counted throwing_cleanup_no_contract (bool arm)
{
  ThrowOnDestroy guard { arm };
  Counted result (7);
  return result;
}

/* Control: a contract on a function whose return type has a trivial
   destructor never creates a sentinel at all.  */
struct Trivial { int v; };
Trivial trivial_pre (int n) pre (n >= 1) { Trivial t { n }; return t; }

static void
check (int got, int want)
{
  if (got != want)
    __builtin_abort ();
}

int
main ()
{
  S s { false };
  S sf { true };

  /* Each call must return the right value and leave nothing alive.  */
  check (s.pre_only (1, 3).v, 4);
  check (live, 0);
  check (sf.pre_only (1, 3).v, 4);
  check (live, 0);

  check (s.post_only (1, 3).v, 4);
  check (live, 0);
  check (s.pre_and_post (1, 3).v, 4);
  check (live, 0);
  check (s.no_contract (1, 3).v, 4);
  check (live, 0);

  check (free_pre (1, 3, false).v, 4);
  check (live, 0);

  /* No throw: ordinary return, nothing left alive.  */
  check (throwing_cleanup_pre (false).v, 7);
  check (live, 0);
  check (throwing_cleanup_no_contract (false).v, 7);
  check (live, 0);

  /* Throw on the way out: the return object must be destroyed exactly once.
     Destroying it twice would drive `live' negative.  */
  try { throwing_cleanup_no_contract (true); } catch (int) {}
  check (live, 0);
  try { throwing_cleanup_pre (true); } catch (int) {}
  check (live, 0);

  check (trivial_pre (2).v, 2);
  check (live, 0);

  return 0;
}
