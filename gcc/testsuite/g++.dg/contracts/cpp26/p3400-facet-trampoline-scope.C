/* Test that a P3400 facet label's trampolines can be generated from every
   context a label may be grokked in, not just at namespace scope.

   Regression test: the trampoline builders define a function via
   start_preparsed_function/finish_function at the point the label is
   grokked.  They saved cfun but not the class scope, so finish_function's
   unconditional "if (current_class_name) ... pop_nested_class ()" popped a
   class binding level that start_preparsed_function had never pushed (the
   trampoline's DECL_CONTEXT is the global namespace), tripping the
   binding-level assertion in poplevel_class.  Every shape below ICEd.

   Each case uses a DISTINCT label type on purpose: trampolines are cached
   per label type, so reusing one label would hit the cache, skip the
   builder entirely, and test nothing.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

int calls = 0;

#define MAKE_LABEL(NAME)						\
  struct NAME##_t {							\
    using assertion_control_object = NAME##_t;				\
    violation_handled							\
    handle_contract_violation (const contract_violation &) const	\
    { ++calls; return violation_handled::handled; }			\
  };									\
  constexpr NAME##_t NAME{}

MAKE_LABEL (lbl_member_pre);
MAKE_LABEL (lbl_member_post);
MAKE_LABEL (lbl_member_outofclass);
MAKE_LABEL (lbl_assert_member);
MAKE_LABEL (lbl_assert_lambda);
MAKE_LABEL (lbl_lambda_post);
MAKE_LABEL (lbl_class_template);
MAKE_LABEL (lbl_class_template_assert);
MAKE_LABEL (lbl_query);

/* A pre on a member function defined inside the class body.  */
struct MemberPre
{
  int f (int x) pre<lbl_member_pre> (x > 0) { return x; }
};

/* A post on a member function defined inside the class body.  */
struct MemberPost
{
  int f (int x) post<lbl_member_post> (r : r > 0) { return x; }
};

/* Declared in the class, defined out of line: the condition is deferred at
   the point the label is grokked, but the label itself never is.  */
struct MemberOutOfClass
{
  int f (int x) pre<lbl_member_outofclass> (x > 0);
};

int
MemberOutOfClass::f (int x)
{
  return x;
}

/* A contract_assert grokked while a member function body is being parsed:
   both a live statement-list stack and a live class scope.  */
struct AssertInMember
{
  void f (int x) { contract_assert<lbl_assert_member> (x > 0); }
};

/* A member of a class template, so the trampoline is built from
   tsubst_contract during instantiation with the class scope live.  */
template <class T>
struct ClassTemplate
{
  T f (T x) pre<lbl_class_template> (x > 0) { return x; }
};

template <class T>
struct ClassTemplateAssert
{
  template <class U>
  void m (U x) { contract_assert<lbl_class_template_assert> (x > 0); }
};

/* A label with a query facet rather than a handler exercises the other
   trampoline builder.  */
struct query_label_t
{
  using assertion_control_object = query_label_t;
  violation_handled
  handle_contract_violation (const contract_violation &) const
  { ++calls; return violation_handled::handled; }
  void *query (const void *, std::size_t) const { return nullptr; }
};
constexpr query_label_t lbl_query_facet{};

struct QueryInClass
{
  int f (int x) pre<lbl_query_facet> (x > 0) { return x; }
};

int
main ()
{
  int expected = 0;

  MemberPre a;
  a.f (-1);
  ++expected;

  MemberPost b;
  b.f (-1);
  ++expected;

  MemberOutOfClass c;
  c.f (-1);
  ++expected;

  AssertInMember d;
  d.f (-1);
  ++expected;

  /* A contract_assert inside a lambda inside a function body.  */
  auto lam = [] (int y) { contract_assert<lbl_assert_lambda> (y > 0); };
  lam (-1);
  ++expected;

  /* A post on a lambda in a function body.  */
  auto lam2 = [] (int y) post<lbl_lambda_post> (r : r > 0) { return y; };
  lam2 (-1);
  ++expected;

  ClassTemplate<int> e;
  e.f (-1);
  ++expected;

  ClassTemplateAssert<int> f;
  f.m (-1);
  ++expected;

  QueryInClass g;
  g.f (-1);
  ++expected;

  if (calls != expected)
    __builtin_abort ();
}
