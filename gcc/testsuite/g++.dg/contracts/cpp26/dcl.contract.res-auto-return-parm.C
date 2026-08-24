/* A precondition naming a parameter, on a function whose return type is
   deduced (auto), ICEd when the function is (or is a member of) a
   template.  Needs actual codegen, not just parsing: -fsyntax-only alone
   compiled clean, since the malformed tree only mattered once genericized
   and gimplified.

   tsubst_contract raises processing_template_decl while substituting a
   contract's condition whenever the enclosing function has a deduced
   return type -- but that is only needed for a POSTcondition, whose
   condition may name the not-yet-typed result variable.  A precondition's
   condition can only ever reference parameters, never the return type, so
   it never needed the raise.  Leaving it raised anyway meant an ordinary,
   fully-resolvable subexpression in the precondition -- here a scalar
   `T()` value-initialization, which is a template-only placeholder
   CAST_EXPR with a NULL operand until it is re-substituted outside a
   template context -- came back from tsubst_expr still in that
   unresolved, template-shaped form.  It then reached the gimplifier
   un-substituted, which dereferenced its NULL operand.

   Reproduces with plain -fcontracts: no P3850/P3097/P3100/P4298 extension
   is involved.  Every crashing shape here is a precondition; the auto+post
   case (which legitimately needs the deferral, since `r`'s type really is
   not yet known) is exercised as a control that must keep working.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;

int violations = 0;

void handle_contract_violation (const contract_violation &)
{
  ++violations;
}

/* --- (1) plain function template, precondition names a parameter ------- */
template <class T> auto ft (T v) pre (v != T ());
template <class T> auto ft (T v) { return v; }

/* --- (2) class-template member, precondition defined INLINE ------------ */
template <class T> struct Inl {
  auto f (T v) pre (v != T ()) { return v; }
};

/* --- (3) class-template member, defined OUT-OF-LINE --------------------- */
template <class T> struct Ool {
  auto f (T v) pre (v != T ());
};
template <class T> auto Ool<T>::f (T v) { return v; }

/* --- controls: each drops exactly one necessary ingredient ------------- */

/* no deduced return type */
template <class T> struct WrittenReturn {
  T f (T v) pre (v != T ());
};
template <class T> T WrittenReturn<T>::f (T v) { return v; }

/* deduced return type, but the predicate names no parameter */
template <class T> struct NoParmInPredicate {
  auto f (T v) pre (sizeof (T) > 0);
};
template <class T> auto NoParmInPredicate<T>::f (T v) { return v; }

/* deduced return type and a parameter in the predicate, but no template */
struct NonTemplate {
  auto f (int v) pre (v != 0);
};
auto NonTemplate::f (int v) { return v; }

/* auto return type with a POSTcondition naming the (deduced) result: the
   legitimate reason the deferral exists, must keep working.  */
template <class T> auto post_auto (const T v) post (r : r == v) { return v; }

int
main ()
{
  violations = 0;
  ft<int> (0);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  Inl<int> i;
  i.f (0);
  if (violations != 1)
    __builtin_abort ();

  violations = 0;
  Ool<int> o;
  o.f (0);
  if (violations != 1)
    __builtin_abort ();

  /* Controls: must compile and must not report a spurious violation.  */
  violations = 0;
  WrittenReturn<int> w;
  w.f (1);
  NoParmInPredicate<int> n;
  n.f (1);
  NonTemplate t;
  t.f (1);
  if (violations != 0)
    __builtin_abort ();

  if (post_auto<int> (5) != 5)
    __builtin_abort ();

  return 0;
}
