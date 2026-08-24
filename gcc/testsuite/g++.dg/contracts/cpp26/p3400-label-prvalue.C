/* A P3400 facet label written as a prvalue -- pre<L{}> -- must have its
   facets applied, just like a named label object.

   Regression test: the runtime descriptor needs the label's address, so
   a prvalue label is materialized into a TU-local constant.  That
   materialization pre-folded the expression with cxx_constant_value and
   passed the result to pushdecl_top_level_and_finish; when the fold
   returned error_mark_node the whole thing was skipped in silence,
   leaving CONTRACT_LABEL a prvalue.  build_contract_check requires a
   VAR_DECL there, so the local-violation handler was never wired up and
   the facet simply did not run -- no diagnostic, the contract just
   behaved as if unlabelled.

   Handing the unfolded expression to cp_finish_decl and letting it do
   the evaluation fixes it, and using the decl pushdecl_namespace_level
   returns rather than the one passed in.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

int calls = 0;

struct L
{
  using assertion_control_object = L;
  violation_handled
  handle_contract_violation (const contract_violation &) const
  { ++calls; return violation_handled::handled; }
};

constexpr L named{};

template <class T> struct wrap { using type = L; };

/* Prvalue label.  */
void prvalue_label (int x) pre<L{}> (x > 0) { }

/* Named label: the control that already worked.  */
void named_label (int x) pre<named> (x > 0) { }

/* Prvalue label in a template, reached through tsubst_contract.  */
template <class T>
int tmpl_prvalue (T x) post<typename wrap<T>::type{}> (r : r > 0)
{ return x; }

/* The same, with a DEDUCED return type.  tsubst_contract raises
   processing_template_decl for that -- to keep the condition from
   tripping over the not-yet-known type -- and the label was substituted
   with it still raised, so a prvalue label came back as the syntactic
   COMPOUND_LITERAL_P CONSTRUCTOR rather than a value and ICEd in
   store_init_value when materialized.  All three of template, prvalue
   label and deduced return type are needed: any two of them are fine.  */
template <class T>
auto tmpl_prvalue_auto (T x) post<typename wrap<T>::type{}> (r : r > 0)
{ return x; }

int
main ()
{
  calls = 0;
  named_label (-1);
  if (calls != 1)
    __builtin_abort ();

  calls = 0;
  prvalue_label (-1);
  if (calls != 1)
    __builtin_abort ();

  calls = 0;
  tmpl_prvalue<int> (-1);
  if (calls != 1)
    __builtin_abort ();

  calls = 0;
  tmpl_prvalue_auto<int> (-1);
  if (calls != 1)
    __builtin_abort ();

  return 0;
}
