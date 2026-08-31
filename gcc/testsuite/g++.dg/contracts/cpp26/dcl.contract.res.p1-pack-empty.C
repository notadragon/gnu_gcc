/* Test the postcondition const requirement on an instantiation whose
   function parameter pack does not line up with the pattern's.

   check_postconditions_in_redecl carries the "odr used in a postcondition"
   property from the pattern's parameters over to the instantiation's and
   checks the const requirement on the latter.  It walked the two parameter
   lists in lockstep, which does not hold across a pack: a pack occupies one
   slot in the pattern but expands to N parameters in the instantiation.
   Three distinct defects followed, all reachable with plain -fcontracts --
   these are base-P2900 defects, not P3850-extension ones:

     * a pack expanding to NOTHING leaves the instantiation's list the
       shorter of the two, so the walk ran off its end and dereferenced null
       -- an ICE, needing no postcondition at all, just any contract
       specifier to get the walk started;

     * the same crash one parameter earlier, when a parameter is written
       after the empty pack: null reached set_parm_used_in_post rather than
       the loop increment;

     * a parameter written after a pack that expands to N sits N-1 slots
       further along in the instantiation, so the walk paired it with a pack
       element instead.  A well-formed program was rejected, blaming the pack
       element for the tail parameter's use.

   Whatever the pack expands to, parameters before the first pack correspond
   from the front and parameters after the last pack correspond from the
   back, so both runs are exercised here in both directions: cases that must
   compile, and cases that must still be diagnosed and must name the
   parameter that is really at fault.

   A parameter whose type is written non-dependently is diagnosed while the
   contract is parsed, before any of this runs, so every case that has to
   reach the instantiation gives its parameter a dependent type.  */

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

/* A contract specifier and a pack that expands to nothing.  No postcondition
   is involved: any contract specifier is enough to run the walk.  */
template <class... A>
void pre_empty (int x, A&&... a)
  pre (x > 0)
{ }

/* The same with a postcondition, and with the pack itself odr-used in one.  */
template <class... A>
int post_empty (const int x, A... a)
  post (r : r > x)
{ return x + 1; }

template <class... A>
int post_empty_uses_pack (const A... a)
  post (r : (int (a) + ... + 0) >= r)
{ return 0; }

/* A parameter written after the pack, const, so nothing is wrong with any
   instantiation of it: the pack element opposite it must not be blamed.  */
template <class... A, class T>
int tail_const (A... a, const T y)
  post (r : r > y)
{ return y + 1; }

/* A parameter after the pack that really is non-const, so the back-aligned
   run must still diagnose -- and must name Y, the parameter actually used,
   not whichever pack element shares its position.

   Each of these is diagnosed twice, from two different places, and the two
   messages are deliberately worded and located differently: this walk names
   the parameter and points at its declaration, while substituting the
   predicate points at the odr-use inside the postcondition and informs about
   the declaration.  Neither has anything to do with packs -- any
   dependent-typed value parameter used in a postcondition gets both -- so
   both are simply matched here rather than worked around.

   A postcondition whose return type is deduced draws a third copy, the
   use-site message repeated at the declaration, from rebuild_postconditions
   re-substituting the predicate.  A written return type gives the result
   variable a concrete type, so that rebuild is skipped and the third copy
   does not appear; every function here writes its return type.  */
template <class... A, class T>
int tail_nonconst (A... a, T y)			// { dg-error "value parameter 'y' used in a postcondition must be const" }
  post (r : r > y)				// { dg-error "a value parameter used in a postcondition must be const" }
{ return y + 1; }

/* The same, reached with an empty pack: the back-aligned run has to cope
   with the instantiation being the shorter list.  */
template <class... A, class T>
int tail_nonconst_empty (A... a, T y)		// { dg-error "value parameter 'y' used in a postcondition must be const" }
  post (r : r > y)				// { dg-error "a value parameter used in a postcondition must be const" }
{ return y + 1; }

/* A parameter before the pack: the front-aligned run keeps working.  */
template <class T, class... A>
int front_const (const T x, A... a)
  post (r : r > x)
{ return x + 1; }

template <class T, class... A>
int front_nonconst (T x, A... a)		// { dg-error "value parameter 'x' used in a postcondition must be const" }
  post (r : r > x)				// { dg-error "a value parameter used in a postcondition must be const" }
{ return x + 1; }

/* Parameters on both sides of the pack at once.  */
template <class T, class... A, class U>
int both_sides (const T x, A... a, const U y)
  post (r : r > x + y)
{ return x + y + 1; }

/* A member of a class template, whose contracts are parsed later.  */
template <class T>
struct holder
{
  template <class... A>
  int f (const T x, A... a)
    post (r : r > x)
  { return x + 1; }
};

/* A separate declaration and definition, so the redeclaration path runs over
   the pattern as well as the instantiation.  */
template <class... A, class T>
int declared_first (A... a, const T y)
  post (r : r > y);

template <class... A, class T>
int
declared_first (A... a, const T y)
{ return y + 1; }

void
g ()
{
  /* Empty packs.  */
  pre_empty (1);
  post_empty (1);
  post_empty_uses_pack ();
  tail_const<> (1);
  front_const (1);
  both_sides<int> (1, 2);
  declared_first<> (1);

  /* Packs expanding to one element: the lists are the same length, but a
     tail parameter is still displaced by the pack.  */
  pre_empty (1, 2);
  post_empty (1, 2);
  post_empty_uses_pack (1);
  tail_const<int> (1, 2);
  front_const (1, 2);
  both_sides<int, int> (1, 2, 3);
  declared_first<int> (1, 2);

  /* Packs expanding to more than one element.  */
  pre_empty (1, 2, 3);
  post_empty (1, 2, 3);
  post_empty_uses_pack (1, 2);
  tail_const<int, int> (1, 2, 3);
  front_const (1, 2, 3);
  both_sides<int, int, int> (1, 2, 3, 4);
  declared_first<int, int> (1, 2, 3);

  holder<int> ().f (1);
  holder<int> ().f (1, 2);

  /* The genuinely non-const cases, each instantiated exactly once.  */
  tail_nonconst<int, int> (1, 2, 3);
  tail_nonconst_empty<> (1);
  front_nonconst (1, 2);
}
