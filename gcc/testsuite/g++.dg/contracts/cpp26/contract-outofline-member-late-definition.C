/* A contract on a member of a class template ICEd when the member was
   defined out of line and the class was completed before that definition was
   seen:

     internal compiler error: in tsubst_expr, at cp/pt.cc:23351

   The member has two declarations, and the predicate names the parameters of
   the in-class one.  Merging the out-of-line definition replaces the
   surviving declaration's parameters with the definition's
   (update_contract_arguments, cp/decl.cc).  But an instantiation created
   before that merge -- which is what completing the class early does --
   still holds the copy of the specifier that tsubst_function_decl took at
   the time, naming parameters that are no longer anywhere.  Substitution
   registers the pattern's current parameters, finds no local specialization
   for the ones the condition actually names, and falls into tsubst_expr's
   "parameter used in a late-specified return type" recovery, whose
   gcc_assert (cp_unevaluated_operand) then fires -- a contract predicate is
   evaluated, not unevaluated.

   Fixed by taking the specifiers from the same declaration whose parameters
   substitution registers, so the two cannot disagree.

   What matters is that the predicate names a PARAMETER: naming only a
   template parameter, a data member, or the postcondition result is fine,
   and `pre' and `post' are affected alike.  Whether the out-of-line
   definition names its parameters is irrelevant.  Note there is no variant
   with the contract written on the definition instead: that is ill-formed
   ("declaration adds contracts to ...").

   Reproduces with plain -fcontracts: no P3850/P3097/P3100/P4298 extension is
   involved, and no P3400 label.  Needs no codegen -- -fsyntax-only alone
   ICEd.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations = 0;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

/* --- (1) the crashing shape: class completed before the definition ------ */
template <class T> struct Pre {
  void f (int a) pre (a > 0);
};
Pre<int> early_pre;				/* completes Pre<int> HERE */
template <class T> void Pre<T>::f (int) { }	/* ... before this */

/* --- (2) postcondition naming a parameter, same shape ------------------- */
template <class T> struct Post {
  int g (const int a) post (r : r != a);
};
Post<int> early_post;
template <class T> int Post<T>::g (const int a) { return a == 0 ? 0 : a + 1; }

/* --- (3) both on one member; definition names its parameters ------------ */
template <class T> struct Both {
  int h (const int a, int b) pre (a <= b) post (r : r >= a);
};
Both<int> early_both;
template <class T> int Both<T>::h (const int a, int b) { return a + b - b; }

/* --- (4) the real-world trigger: an explicit member specialization of an
       unrelated member completes the class as a side effect --------------- */
template <class T> struct Spec {
  void other (int);
  void f (int a) pre (a > 0);
};
template <class T> void Spec<T>::other (int) { }
template <> void Spec<int>::other (int) { }	/* completes Spec<int> here */
template <class T> void Spec<T>::f (int) { }

/* --- controls: each drops exactly one necessary ingredient -------------- */

/* defined in-class, so there is only ever one declaration */
template <class T> struct InClass {
  void f (int a) pre (a > 0) { }
};
InClass<int> early_inclass;

/* defined out of line, but the class is not completed until afterwards */
template <class T> struct LateComplete {
  void f (int a) pre (a > 0);
};
template <class T> void LateComplete<T>::f (int) { }

/* the predicate names no parameter */
template <class T> struct NoParm {
  int d;
  void f (int a) pre (sizeof (T) > 0) pre (d >= 0);
};
NoParm<int> early_noparm;
template <class T> void NoParm<T>::f (int) { }

/* a postcondition naming only the result */
template <class T> struct ResultOnly {
  int f (int a) post (r : r >= 0);
};
ResultOnly<int> early_result;
template <class T> int ResultOnly<T>::f (int a) { return a; }

/* not a member of a class template */
template <class T> void free_fn (int a) pre (a > 0);
template <class T> void free_fn (int) { }

/* not a template at all */
struct NonTemplate {
  void f (int a) pre (a > 0);
};
void NonTemplate::f (int) { }

template struct Pre<int>;
template struct Post<int>;
template struct Both<int>;
template struct Spec<int>;
template struct LateComplete<int>;
template struct NoParm<int>;
template struct ResultOnly<int>;
template void free_fn<int> (int);

static void
check (int got, int want)
{
  if (got != want)
    __builtin_abort ();
}

int
main ()
{
  /* Each contract must actually be checked, and against the right parameter:
     a fix that registered the wrong declaration's parameters could still
     compile and then silently test garbage.  */
  violations = 0;
  early_pre.f (1);
  check (violations, 0);
  early_pre.f (0);
  check (violations, 1);

  violations = 0;
  check (early_post.g (1), 2);
  check (violations, 0);		/* r != a is 2 != 1: holds */
  check (early_post.g (0), 0);
  check (violations, 1);		/* r != a is 0 != 0: fails */

  violations = 0;
  check (early_both.h (1, 2), 1);
  check (violations, 0);
  check (early_both.h (2, 1), 2);
  check (violations, 1);		/* pre a <= b fails, post r >= a holds */

  violations = 0;
  Spec<int> s;
  s.f (1);
  check (violations, 0);
  s.f (0);
  check (violations, 1);

  /* Controls.  */
  violations = 0;
  early_inclass.f (0);
  LateComplete<int> lc;
  lc.f (0);
  NonTemplate nt;
  nt.f (0);
  free_fn<int> (0);
  check (violations, 4);

  violations = 0;
  early_noparm.d = 0;
  early_noparm.f (0);			/* both predicates hold */
  check (early_result.f (1), 1);
  check (violations, 0);

  return 0;
}
