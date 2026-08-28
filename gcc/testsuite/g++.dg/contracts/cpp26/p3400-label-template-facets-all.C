/* Test that a DEPENDENT P3400 label's remaining facets take effect at
   instantiation, not just compute_semantic.

   Regression test: tsubst_contract gained label substitution, which fixed
   compute_semantic, allowed_semantics, local_violation_label and
   queryable_label.  Two facets were still silently dropped:

     - compute_comment / compute_message.  apply_label_string_facet is
       only ever reached from the parse-time paths (grok_contract,
       update_late_contract, finish_contract_message), all of which bail
       on a dependent label, and tsubst_contract never re-applied them.
       The comment stayed the raw predicate text.

     - group_names.  Worse than "never ran": caller-side resolution runs
       at the call site, before the definition is instantiated, so
       ensure_contract_groups saw the still-dependent pattern label and
       cached error_mark_node in CONTRACT_GROUPS.  copy_node in
       tsubst_contract then carried that poison onto the instantiation,
       where it made every later group lookup early-out -- so a
       group-based configuration silently did not apply.

   The label has to be DEPENDENT for either bug to show.  A label that is
   already concrete at parse time -- a plain namespace-scope constexpr
   object, or an inline "name"group literal -- resolves normally even on a
   function template, so a test written with one of those would pass with
   or without the fix.  Each case therefore spells the label as a variable
   template indexed by T, with a non-template control alongside.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=quietgroup:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

using std::contracts::contract_violation;

int violations = 0;
const char *last_comment = nullptr;

void
handle_contract_violation (const contract_violation &v)
{
  ++violations;
  last_comment = v.comment ();
}

/* compute_comment: rewrites the reported comment text.  */
struct comment_label_t
{
  using assertion_control_object = comment_label_t;
  constexpr const char *compute_comment (const char *) const
  { return "REWRITTEN"; }
};
constexpr comment_label_t comment_label{};

/* Dependent spellings of labels that are themselves perfectly concrete.  */
template <class T> constexpr comment_label_t dep_comment_label{};
struct quiet_label_t
{
  using assertion_control_object = quiet_label_t;
  const char group_names[1][11];
  constexpr quiet_label_t () : group_names{"quietgroup"} {}
};
template <class T> constexpr quiet_label_t dep_quiet_label{};

void nontmpl_comment (int x) pre<comment_label> (x > 0) { }

template <class T>
void tmpl_comment (T x) pre<dep_comment_label<T>> (x > 0) { }

void nontmpl_group (int x) pre<"quietgroup"group> (x > 0) { }

template <class T>
void tmpl_group (T x) pre<dep_quiet_label<T>> (x > 0) { }

int
main ()
{
  /* compute_comment must rewrite the comment in both cases.  */
  violations = 0;
  last_comment = nullptr;
  nontmpl_comment (-1);
  if (violations != 1 || !last_comment
      || std::strcmp (last_comment, "REWRITTEN") != 0)
    __builtin_abort ();

  violations = 0;
  last_comment = nullptr;
  tmpl_comment (-1);
  if (violations != 1 || !last_comment
      || std::strcmp (last_comment, "REWRITTEN") != 0)
    __builtin_abort ();

  /* group_names must place both in quietgroup, which the command line
     configures to ignore, so neither fires at all.  */
  violations = 0;
  nontmpl_group (-1);
  if (violations != 0)
    __builtin_abort ();

  violations = 0;
  tmpl_group (-1);
  if (violations != 0)
    __builtin_abort ();

  return 0;
}
