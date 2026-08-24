/* A P3400 facet label written as a prvalue -- pre<L{}> -- inside a TEMPLATE
   must have its facets applied, just as it does outside one.

   Regression test.  A prvalue label whose type carries a local-violation or
   query facet needs its address at run time, so resolve_contract_label
   materializes it into a TU-local constant with
   pushdecl_top_level_and_finish.  That clears processing_template_decl
   underneath the initializer, which -- when the contract is being parsed
   inside a template -- is still in template form.  Two ICEs followed:

     * a non-dependent prvalue label is an undigested COMPOUND_LITERAL_P
       CONSTRUCTOR, so store_init_value asserted (cp/typeck2.cc);
     * a label with a dependent subtree got constant-evaluated with template
       trees still in it, so dependent_type_p asserted (cp/pt.cc).

   All three ingredients are needed and each is varied below: the label must
   be a prvalue (a named constexpr object is already a VAR_DECL), it must
   carry a facet that needs its address (a label with only compute_semantic
   is never materialized), and the contract must be inside a template.
   Neither the label nor its type has to be a template itself.

   p3400-label-prvalue.C covers the same materialization outside a template
   and with a *type-dependent* label in one; a type-dependent label returns
   from resolve_contract_label at parse time and is only ever resolved from
   tsubst_contract, which is why that test did not catch this.

   Found by the BDE contracts integration, where BSLS_ASSERT expands to
   contract_assert carrying a label class template.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;
using std::contracts::violation_handled;

static int local_calls = 0;
static int global_calls = 0;
static const char *last_tag = nullptr;

/* Label carrying a local-violation facet: needs its address, handles the
   violation itself so the global handler never sees it.  Not a template,
   to show the label type need not be one.  */
struct local_label
{
  using assertion_control_object = local_label;
  violation_handled
  handle_contract_violation (const contract_violation &) const
  { ++local_calls; return violation_handled::handled; }
};

constexpr local_label named{};

/* Label class template with a query facet -- the shape BSLS_ASSERT uses.  */
template <evaluation_semantic SEM>
struct tmpl_label
{
  using assertion_control_object = tmpl_label;
  static constexpr int key = 0;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return SEM; }
  void *query (const void *k, std::size_t i) const
  { return (i == 0 && k == &key) ? (void *) "TMPL" : nullptr; }
};

using observe_label = tmpl_label<evaluation_semantic::observe>;

/* Query label whose tag comes from the expression, so a label built from a
   template parameter is value-dependent without being type-dependent.  */
struct tag_label
{
  using assertion_control_object = tag_label;
  static constexpr int key = 0;
  const char *tag;
  constexpr tag_label (const char *t) : tag (t) { }
  void *query (const void *k, std::size_t i) const
  { return (i == 0 && k == &key) ? (void *) tag : nullptr; }
};

/* Facet-less label: never materialized, so it exercises the path the fix
   must leave alone.  */
struct ignore_label
{
  using assertion_control_object = ignore_label;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::ignore; }
};

void
handle_contract_violation (const contract_violation &v)
{
  ++global_calls;
  last_tag = (const char *) v.query_control_object (&tag_label::key);
  if (!last_tag)
    last_tag = (const char *) v.query_control_object (&observe_label::key);
}

/* Outside a template: the control that already worked.  */
void nontmpl_pre (int x) pre<local_label{}> (x > 0) { }

/* Named label inside a template: the other control -- already a VAR_DECL,
   so nothing is materialized.  */
template <class T> void tmpl_named (T x) pre<named> (x > 0) { }

/* The three contract kinds, each with a non-dependent prvalue label.  */
template <class T> void tmpl_pre (T x) pre<local_label{}> (x > 0) { }
template <class T> void tmpl_assert (T x)
{ contract_assert<local_label{}> (x > 0); }
template <class T> T tmpl_post (T x) post<local_label{}> (r : r > 0)
{ return x; }

/* Deduced return type as well: tsubst_contract lowers
   processing_template_decl for the label on that path, so make sure the
   parse-time path and that one agree.  */
template <class T> auto tmpl_auto_post (T x) post<local_label{}> (r : r > 0)
{ return x; }

/* Members of a class template, including a contract_assert in the body --
   the shape that ICEd in bsls_nameof.h.  */
template <class T>
struct holder
{
  static void check (T x) pre<local_label{}> (x > 0) { }
  static void bde (T x) { contract_assert<observe_label{}> (x > 0); }
};

/* Label class template specialization materialized at the expansion site.  */
template <class T> void tmpl_bde (T x)
{ contract_assert<observe_label{}> (x > 0); }

/* Value-dependent, not type-dependent: one TU-local constant cannot serve
   both instantiations, so each must get its own.  */
template <int N> void tmpl_vdep (int x)
{ contract_assert<tag_label{N == 1 ? "ONE" : "TWO"}> (x > 0); }

/* No facet needing an address: nothing is materialized, but compute_semantic
   must still apply and switch the contract off.  */
template <class T> void tmpl_ignore (T x) pre<ignore_label{}> (x > 0) { }

static void
expect_local (int expected_local)
{
  if (local_calls != expected_local || global_calls != 0)
    __builtin_abort ();
}

static void
expect_tag (const char *expected)
{
  if (global_calls != 1 || local_calls != 0)
    __builtin_abort ();
  if (!last_tag || std::strcmp (last_tag, expected) != 0)
    __builtin_abort ();
}

static void
reset ()
{
  local_calls = 0;
  global_calls = 0;
  last_tag = nullptr;
}

int
main ()
{
  reset (); nontmpl_pre (-1);        expect_local (1);
  reset (); tmpl_named (-1);         expect_local (1);

  reset (); tmpl_pre (-1);           expect_local (1);
  reset (); tmpl_assert (-1);        expect_local (1);
  reset (); tmpl_post (-1);          expect_local (1);
  reset (); tmpl_auto_post (-1);     expect_local (1);
  reset (); holder<int>::check (-1); expect_local (1);

  reset (); tmpl_bde (-1);           expect_tag ("TMPL");
  reset (); holder<int>::bde (-1);   expect_tag ("TMPL");

  /* Each instantiation carries its own label object.  */
  reset (); tmpl_vdep<1> (-1);       expect_tag ("ONE");
  reset (); tmpl_vdep<2> (-1);       expect_tag ("TWO");

  reset (); tmpl_ignore (-1);
  if (local_calls != 0 || global_calls != 0)
    __builtin_abort ();

  return 0;
}
